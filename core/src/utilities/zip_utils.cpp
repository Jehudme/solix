#include "solix/zip_utils.hpp"
#include <miniz.h>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace solix {

namespace {

inline std::string normalize_archive_path(const std::string& path_str) {
    std::string norm = path_str;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    while (!norm.empty() && norm.front() == '/') {
        norm.erase(norm.begin());
    }
    return norm;
}

} // namespace

// ============================================================================
// ZipArchiveWriter
// ============================================================================

struct ZipArchiveWriter::Impl {
    mz_zip_archive zip;
    bool is_open = false;
    bool finalized = false;
    std::string error_message;
    std::filesystem::path output_path;

    Impl(const std::filesystem::path& out_path) : output_path(out_path) {
        std::memset(&zip, 0, sizeof(zip));
        std::error_code ec;
        if (output_path.has_parent_path()) {
            std::filesystem::create_directories(output_path.parent_path(), ec);
        }

        mz_bool res = mz_zip_writer_init_file(&zip, output_path.string().c_str(), 0);
        if (!res) {
            mz_zip_error err = mz_zip_get_last_error(&zip);
            error_message = "Failed to initialize zip writer: " + std::string(mz_zip_get_error_string(err));
            is_open = false;
        } else {
            is_open = true;
        }
    }

    ~Impl() {
        if (is_open) {
            if (!finalized) {
                mz_zip_writer_finalize_archive(&zip);
            }
            mz_zip_writer_end(&zip);
            is_open = false;
        }
    }
};

ZipArchiveWriter::ZipArchiveWriter(const std::filesystem::path& output_path)
    : impl_(std::make_unique<Impl>(output_path)) {}

ZipArchiveWriter::~ZipArchiveWriter() = default;
ZipArchiveWriter::ZipArchiveWriter(ZipArchiveWriter&&) noexcept = default;
ZipArchiveWriter& ZipArchiveWriter::operator=(ZipArchiveWriter&&) noexcept = default;

bool ZipArchiveWriter::is_valid() const {
    return impl_ && impl_->is_open && impl_->error_message.empty();
}

const std::string& ZipArchiveWriter::get_error() const {
    static const std::string empty;
    return impl_ ? impl_->error_message : empty;
}

bool ZipArchiveWriter::add_file(const std::string& archive_name, const std::filesystem::path& disk_path) {
    if (!is_valid() || impl_->finalized) {
        if (impl_) impl_->error_message = "Archive writer is closed or finalized";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(disk_path, ec) || !std::filesystem::is_regular_file(disk_path, ec)) {
        impl_->error_message = "Source file does not exist: " + disk_path.string();
        return false;
    }

    std::string norm_name = normalize_archive_path(archive_name);
    mz_bool res = mz_zip_writer_add_file(&impl_->zip, norm_name.c_str(), disk_path.string().c_str(),
                                         nullptr, 0, MZ_DEFAULT_LEVEL);
    if (!res) {
        mz_zip_error err = mz_zip_get_last_error(&impl_->zip);
        impl_->error_message = "Failed to add file '" + disk_path.string() + "': " +
                               std::string(mz_zip_get_error_string(err));
        return false;
    }
    return true;
}

bool ZipArchiveWriter::add_buffer(const std::string& archive_name, const void* data, size_t size) {
    if (!is_valid() || impl_->finalized) {
        if (impl_) impl_->error_message = "Archive writer is closed or finalized";
        return false;
    }

    std::string norm_name = normalize_archive_path(archive_name);
    mz_bool res = mz_zip_writer_add_mem(&impl_->zip, norm_name.c_str(), data, size, MZ_DEFAULT_LEVEL);
    if (!res) {
        mz_zip_error err = mz_zip_get_last_error(&impl_->zip);
        impl_->error_message = "Failed to add memory buffer '" + norm_name + "': " +
                               std::string(mz_zip_get_error_string(err));
        return false;
    }
    return true;
}

bool ZipArchiveWriter::add_string(const std::string& archive_name, const std::string& content) {
    return add_buffer(archive_name, content.data(), content.size());
}

bool ZipArchiveWriter::add_directory_recursive(const std::filesystem::path& dir_path, const std::string& archive_prefix) {
    if (!is_valid() || impl_->finalized) {
        if (impl_) impl_->error_message = "Archive writer is closed or finalized";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec)) {
        return true; // Empty or absent directory is a no-op
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path, ec)) {
        if (entry.is_regular_file(ec)) {
            std::filesystem::path rel = std::filesystem::relative(entry.path(), dir_path, ec);
            std::string arc_name;
            if (archive_prefix.empty()) {
                arc_name = rel.generic_string();
            } else {
                arc_name = (std::filesystem::path(archive_prefix) / rel).generic_string();
            }
            if (!add_file(arc_name, entry.path())) {
                return false;
            }
        }
    }
    return true;
}

bool ZipArchiveWriter::finalize() {
    if (!is_valid()) return false;
    if (impl_->finalized) return true;

    mz_bool res = mz_zip_writer_finalize_archive(&impl_->zip);
    if (!res) {
        mz_zip_error err = mz_zip_get_last_error(&impl_->zip);
        impl_->error_message = "Failed to finalize archive: " + std::string(mz_zip_get_error_string(err));
        return false;
    }
    impl_->finalized = true;
    mz_zip_writer_end(&impl_->zip);
    impl_->is_open = false;
    return true;
}

// ============================================================================
// ZipArchiveReader
// ============================================================================

struct ZipArchiveReader::Impl {
    mz_zip_archive zip;
    bool is_open = false;
    std::string error_message;
    std::filesystem::path zip_path;

    Impl(const std::filesystem::path& path) : zip_path(path) {
        std::memset(&zip, 0, sizeof(zip));
        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
            error_message = "ZIP archive file does not exist: " + path.string();
            is_open = false;
            return;
        }

        mz_bool res = mz_zip_reader_init_file(&zip, path.string().c_str(), 0);
        if (!res) {
            mz_zip_error err = mz_zip_get_last_error(&zip);
            error_message = "Failed to initialize zip reader: " + std::string(mz_zip_get_error_string(err));
            is_open = false;
        } else {
            is_open = true;
        }
    }

    ~Impl() {
        if (is_open) {
            mz_zip_reader_end(&zip);
            is_open = false;
        }
    }
};

ZipArchiveReader::ZipArchiveReader(const std::filesystem::path& zip_path)
    : impl_(std::make_unique<Impl>(zip_path)) {}

ZipArchiveReader::~ZipArchiveReader() = default;
ZipArchiveReader::ZipArchiveReader(ZipArchiveReader&&) noexcept = default;
ZipArchiveReader& ZipArchiveReader::operator=(ZipArchiveReader&&) noexcept = default;

bool ZipArchiveReader::is_valid() const {
    return impl_ && impl_->is_open && impl_->error_message.empty();
}

const std::string& ZipArchiveReader::get_error() const {
    static const std::string empty;
    return impl_ ? impl_->error_message : empty;
}

size_t ZipArchiveReader::get_file_count() const {
    if (!is_valid()) return 0;
    return static_cast<size_t>(mz_zip_reader_get_num_files(&impl_->zip));
}

std::vector<std::string> ZipArchiveReader::get_entry_names() const {
    std::vector<std::string> entries;
    if (!is_valid()) return entries;

    mz_uint count = mz_zip_reader_get_num_files(&impl_->zip);
    entries.reserve(count);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat;
        if (mz_zip_reader_file_stat(&impl_->zip, i, &stat)) {
            entries.emplace_back(stat.m_filename);
        }
    }
    return entries;
}

bool ZipArchiveReader::has_entry(const std::string& entry_name) const {
    if (!is_valid()) return false;
    std::string norm = normalize_archive_path(entry_name);
    int idx = mz_zip_reader_locate_file(&impl_->zip, norm.c_str(), nullptr, 0);
    return idx >= 0;
}

bool ZipArchiveReader::extract_all(const std::filesystem::path& destination_dir) const {
    if (!is_valid()) return false;

    std::error_code ec;
    std::filesystem::create_directories(destination_dir, ec);

    mz_uint count = mz_zip_reader_get_num_files(&impl_->zip);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&impl_->zip, i, &stat)) {
            impl_->error_message = "Failed to query file stat in zip";
            return false;
        }

        std::string filename_str = stat.m_filename;
        if (stat.m_is_directory) {
            std::filesystem::path dir = destination_dir / filename_str;
            std::filesystem::create_directories(dir, ec);
            continue;
        }

        std::filesystem::path dest_file = destination_dir / filename_str;
        if (dest_file.has_parent_path()) {
            std::filesystem::create_directories(dest_file.parent_path(), ec);
        }

        mz_bool res = mz_zip_reader_extract_to_file(&impl_->zip, i, dest_file.string().c_str(), 0);
        if (!res) {
            mz_zip_error err = mz_zip_get_last_error(&impl_->zip);
            impl_->error_message = "Failed to extract '" + filename_str + "': " +
                                   std::string(mz_zip_get_error_string(err));
            return false;
        }
    }
    return true;
}

bool ZipArchiveReader::extract_entry(const std::string& entry_name, const std::filesystem::path& destination_file) const {
    if (!is_valid()) return false;
    std::string norm = normalize_archive_path(entry_name);
    int idx = mz_zip_reader_locate_file(&impl_->zip, norm.c_str(), nullptr, 0);
    if (idx < 0) {
        impl_->error_message = "Entry not found in archive: " + entry_name;
        return false;
    }

    std::error_code ec;
    if (destination_file.has_parent_path()) {
        std::filesystem::create_directories(destination_file.parent_path(), ec);
    }

    mz_bool res = mz_zip_reader_extract_to_file(&impl_->zip, static_cast<mz_uint>(idx), destination_file.string().c_str(), 0);
    if (!res) {
        mz_zip_error err = mz_zip_get_last_error(&impl_->zip);
        impl_->error_message = "Failed to extract '" + entry_name + "': " +
                               std::string(mz_zip_get_error_string(err));
        return false;
    }
    return true;
}

std::optional<std::string> ZipArchiveReader::read_entry_as_string(const std::string& entry_name) const {
    if (!is_valid()) return std::nullopt;
    std::string norm = normalize_archive_path(entry_name);
    int idx = mz_zip_reader_locate_file(&impl_->zip, norm.c_str(), nullptr, 0);
    if (idx < 0) {
        return std::nullopt;
    }

    size_t size = 0;
    void* p = mz_zip_reader_extract_to_heap(&impl_->zip, static_cast<mz_uint>(idx), &size, 0);
    if (!p) {
        return std::nullopt;
    }

    std::string str(static_cast<const char*>(p), size);
    mz_free(p);
    return str;
}

std::optional<std::vector<uint8_t>> ZipArchiveReader::read_entry_as_bytes(const std::string& entry_name) const {
    if (!is_valid()) return std::nullopt;
    std::string norm = normalize_archive_path(entry_name);
    int idx = mz_zip_reader_locate_file(&impl_->zip, norm.c_str(), nullptr, 0);
    if (idx < 0) {
        return std::nullopt;
    }

    size_t size = 0;
    void* p = mz_zip_reader_extract_to_heap(&impl_->zip, static_cast<mz_uint>(idx), &size, 0);
    if (!p) {
        return std::nullopt;
    }

    const uint8_t* byte_ptr = static_cast<const uint8_t*>(p);
    std::vector<uint8_t> result(byte_ptr, byte_ptr + size);
    mz_free(p);
    return result;
}

bool is_zip_archive(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    uint8_t header[4];
    f.read(reinterpret_cast<char*>(header), 4);
    if (f.gcount() < 4) return false;
    // PK\x03\x04 (Local file header) or PK\x05\x06 (Empty archive EOCD)
    return (header[0] == 0x50 && header[1] == 0x4B &&
            ((header[2] == 0x03 && header[3] == 0x04) ||
             (header[2] == 0x05 && header[3] == 0x06)));
}

} // namespace solix
