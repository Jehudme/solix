#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace solix {

/**
 * @brief Writer class for creating standard ZIP (.slxpac) archives using miniz.
 */
class ZipArchiveWriter {
public:
    explicit ZipArchiveWriter(const std::filesystem::path& output_path);
    ~ZipArchiveWriter();

    ZipArchiveWriter(const ZipArchiveWriter&) = delete;
    ZipArchiveWriter& operator=(const ZipArchiveWriter&) = delete;

    ZipArchiveWriter(ZipArchiveWriter&&) noexcept;
    ZipArchiveWriter& operator=(ZipArchiveWriter&&) noexcept;

    bool is_valid() const;
    const std::string& get_error() const;

    /**
     * @brief Adds a file from disk into the archive.
     * @param archive_name Path inside the ZIP (e.g. "bytecode.slxbin" or "resources/config.json").
     * @param disk_path Path to the source file on the host filesystem.
     */
    bool add_file(const std::string& archive_name, const std::filesystem::path& disk_path);

    /**
     * @brief Adds a memory buffer into the archive.
     */
    bool add_buffer(const std::string& archive_name, const void* data, size_t size);

    /**
     * @brief Adds string content as a file into the archive.
     */
    bool add_string(const std::string& archive_name, const std::string& content);

    /**
     * @brief Recursively adds all regular files from a directory on disk.
     * @param dir_path Directory on disk.
     * @param archive_prefix Path prefix inside the ZIP (e.g. "resources").
     */
    bool add_directory_recursive(const std::filesystem::path& dir_path, const std::string& archive_prefix = "");

    /**
     * @brief Finalizes the archive and writes out the central directory.
     */
    bool finalize();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Reader class for inspecting and extracting ZIP (.slxpac) archives using miniz.
 */
class ZipArchiveReader {
public:
    explicit ZipArchiveReader(const std::filesystem::path& zip_path);
    ~ZipArchiveReader();

    ZipArchiveReader(const ZipArchiveReader&) = delete;
    ZipArchiveReader& operator=(const ZipArchiveReader&) = delete;

    ZipArchiveReader(ZipArchiveReader&&) noexcept;
    ZipArchiveReader& operator=(ZipArchiveReader&&) noexcept;

    bool is_valid() const;
    const std::string& get_error() const;

    size_t get_file_count() const;
    std::vector<std::string> get_entry_names() const;
    bool has_entry(const std::string& entry_name) const;

    /**
     * @brief Extracts all entries in the ZIP to a destination directory on disk.
     */
    bool extract_all(const std::filesystem::path& destination_dir) const;

    /**
     * @brief Extracts a single entry to a destination file on disk.
     */
    bool extract_entry(const std::string& entry_name, const std::filesystem::path& destination_file) const;

    /**
     * @brief Reads a single entry from the ZIP into a string buffer.
     */
    std::optional<std::string> read_entry_as_string(const std::string& entry_name) const;

    /**
     * @brief Reads a single entry from the ZIP into a byte vector.
     */
    std::optional<std::vector<uint8_t>> read_entry_as_bytes(const std::string& entry_name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Checks whether the given file starts with the standard ZIP magic signature.
 */
bool is_zip_archive(const std::filesystem::path& path);

} // namespace solix
