#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Core", "[solixlib][collections]") {
    SECTION("Case 7.1: Custom IIterator and IIterable traversal") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IIterator;
            import solix.collections.IIterable;
            import solix.core.IStringable;
            import solix.core.Int;

            class SimpleIterator implements IIterator {
                private int32[] data;
                private int32 index;
                private int32 count;

                public SimpleIterator(int32[] items, int32 count) {
                    this.data = items;
                    this.count = count;
                    this.index = 0;
                }

                public bool has_next() {
                    return this.index < this.count;
                }

                public IStringable next() {
                    if (this.index >= this.count) {
                        return null;
                    }
                    Int res = new Int(this.data[this.index]);
                    this.index = this.index + 1;
                    return res;
                }
            }

            class SimpleIterable implements IIterable {
                private int32[] items;
                private int32 count;

                public SimpleIterable() {
                    this.items = new int32[3];
                    this.items[0] = 10;
                    this.items[1] = 20;
                    this.items[2] = 30;
                    this.count = 3;
                }

                public IIterator iterator() {
                    return new SimpleIterator(this.items, this.count);
                }
            }

            class Main {
                public static int32 main() {
                    SimpleIterable seq = new SimpleIterable();
                    IIterator it = seq.iterator();

                    if (!it.has_next()) return 1;
                    Int first = (Int)it.next();
                    if (first == null || first.value() != 10) return 2;

                    if (!it.has_next()) return 3;
                    Int second = (Int)it.next();
                    if (second == null || second.value() != 20) return 4;

                    if (!it.has_next()) return 5;
                    Int third = (Int)it.next();
                    if (third == null || third.value() != 30) return 6;

                    if (it.has_next()) return 7;
                    return 0;
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 7.2: ICollection contract and Collections.to_string() formatting") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IIterator;
            import solix.collections.IIterable;
            import solix.collections.ICollection;
            import solix.collections.Collections;
            import solix.core.Int;
            import solix.core.String;
            import solix.core.IStringable;
            import solix.system.Console;

            class ArrayCollection implements ICollection {
                private IStringable[] elements;
                private int32 count;

                public ArrayCollection() {
                    this.elements = new IStringable[10];
                    this.count = 0;
                }

                public int32 size() { return this.count; }
                public bool is_empty() { return this.count == 0; }

                public void clear() {
                    this.count = 0;
                }

                public void add(IStringable item) {
                    this.elements[this.count] = item;
                    this.count = this.count + 1;
                }

                public bool contains(IStringable item) {
                    for (int32 i = 0; i < this.count; i = i + 1) {
                        if (this.elements[i] != null && this.elements[i].to_string().equals(item.to_string())) {
                            return true;
                        }
                    }
                    return false;
                }

                public IStringable[] to_array() {
                    IStringable[] copy = new IStringable[this.count];
                    for (int32 i = 0; i < this.count; i = i + 1) {
                        copy[i] = this.elements[i];
                    }
                    return copy;
                }

                public IIterator iterator() {
                    return new ArrayCollIterator(this.elements, this.count);
                }

                public String to_string() {
                    return Collections.to_string(this);
                }
            }

            class ArrayCollIterator implements IIterator {
                private IStringable[] arr;
                private int32 cnt;
                private int32 pos;

                public ArrayCollIterator(IStringable[] a, int32 c) {
                    this.arr = a;
                    this.cnt = c;
                    this.pos = 0;
                }

                public bool has_next() { return this.pos < this.cnt; }
                public IStringable next() {
                    IStringable val = this.arr[this.pos];
                    this.pos = this.pos + 1;
                    return val;
                }
            }

            class Main {
                public static int32 main() {
                    ArrayCollection col = new ArrayCollection();
                    if (!col.is_empty()) return 1;
                    if (col.size() != 0) return 2;

                    // Empty to_string
                    String sEmpty = col.to_string();
                    if (!sEmpty.equals_chars("[]")) return 3;

                    Int c1 = new Int(1);
                    Int c2 = new Int(2);
                    Int c3 = new Int(3);
                    col.add(c1);
                    col.add(c2);
                    col.add(c3);

                    if (col.is_empty()) return 4;
                    if (col.size() != 3) return 5;
                    if (!col.contains(c2)) return 6;
                    Int c99 = new Int(99);
                    if (col.contains(c99)) return 7;

                    // Formatted to_string
                    String sFull = col.to_string();
                    if (!sFull.equals_chars("[1, 2, 3]")) return 8;

                    // to_array
                    IStringable[] arr = col.to_array();
                    if (arr.length != 3) return 9;
                    Int arr1 = (Int)arr[1];
                    if (arr1.value() != 2) return 10;

                    // clear
                    col.clear();
                    if (!col.is_empty()) return 11;
                    if (col.size() != 0) return 12;

                    return 0;
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 7.3: Polymorphic interface dispatch and Console printing") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IIterable;
            import solix.collections.IIterator;
            import solix.collections.ICollection;
            import solix.collections.Collections;
            import solix.core.Int;
            import solix.core.String;
            import solix.core.IStringable;
            import solix.system.Console;

            class MockCollection implements ICollection {
                public int32 size() { return 2; }
                public bool is_empty() { return false; }
                public bool contains(IStringable item) { return true; }
                public void clear() {}
                public IStringable[] to_array() { return new IStringable[0]; }
                public IIterator iterator() { return new MockIterator(); }
                public String to_string() { return new String("[mock1, mock2]"); }
            }

            class MockIterator implements IIterator {
                private int32 idx;
                public MockIterator() { this.idx = 0; }
                public bool has_next() { return this.idx < 2; }
                public IStringable next() {
                    this.idx = this.idx + 1;
                    return new Int(this.idx);
                }
            }

            class Main {
                public static int32 main() {
                    MockCollection mock = new MockCollection();

                    // Assign to ICollection
                    ICollection colRef = mock;
                    if (colRef.size() != 2) return 1;

                    // Assign to IIterable
                    IIterable iterRef = mock;
                    IIterator it = iterRef.iterator();
                    if (!it.has_next()) return 2;

                    // Assign to IStringable
                    IStringable strRef = mock;
                    String repr = strRef.to_string();
                    if (!repr.equals_chars("[mock1, mock2]")) return 3;

                    // Test Console printing
                    Console.println(mock);
                    Console.print(mock);

                    return 0;
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 7.4: IList contract indexed operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IList;
            import solix.collections.IIterator;
            import solix.collections.Collections;
            import solix.core.Int;
            import solix.core.IStringable;
            import solix.core.String;

            class SimpleList implements IList {
                private IStringable[] data;
                private int32 count;

                public SimpleList() {
                    this.data = new IStringable[10];
                    this.count = 0;
                }

                public int32 size() { return this.count; }
                public bool is_empty() { return this.count == 0; }
                public void clear() { this.count = 0; }

                public bool contains(IStringable item) {
                    return this.index_of(item) != -1;
                }

                public IStringable[] to_array() {
                    IStringable[] r = new IStringable[this.count];
                    for (int32 i = 0; i < this.count; i = i + 1) r[i] = this.data[i];
                    return r;
                }

                public IIterator iterator() {
                    return new SimpleListIterator(this.data, this.count);
                }

                public String to_string() {
                    return Collections.to_string(this);
                }

                public void add(IStringable item) {
                    this.data[this.count] = item;
                    this.count = this.count + 1;
                }

                public IStringable get(int32 index) {
                    return this.data[index];
                }

                public void set(int32 index, IStringable item) {
                    this.data[index] = item;
                }

                public void insert(int32 index, IStringable item) {
                    for (int32 i = this.count; i > index; i = i - 1) {
                        this.data[i] = this.data[i - 1];
                    }
                    this.data[index] = item;
                    this.count = this.count + 1;
                }

                public bool remove(IStringable item) {
                    int32 idx = this.index_of(item);
                    if (idx == -1) return false;
                    this.remove_at(idx);
                    return true;
                }

                public IStringable remove_at(int32 index) {
                    IStringable old = this.data[index];
                    for (int32 i = index; i < this.count - 1; i = i + 1) {
                        this.data[i] = this.data[i + 1];
                    }
                    this.count = this.count - 1;
                    return old;
                }

                public int32 index_of(IStringable item) {
                    for (int32 i = 0; i < this.count; i = i + 1) {
                        if (this.data[i] != null && this.data[i].to_string().equals(item.to_string())) {
                            return i;
                        }
                    }
                    return -1;
                }
            }

            class SimpleListIterator implements IIterator {
                private IStringable[] arr;
                private int32 cnt;
                private int32 pos;
                public SimpleListIterator(IStringable[] a, int32 c) { this.arr = a; this.cnt = c; this.pos = 0; }
                public bool has_next() { return this.pos < this.cnt; }
                public IStringable next() {
                    IStringable val = this.arr[this.pos];
                    this.pos = this.pos + 1;
                    return val;
                }
            }

            class Main {
                public static int32 main() {
                    IList list = new SimpleList();
                    Int a1 = new Int(10);
                    Int a2 = new Int(20);
                    Int a3 = new Int(30);
                    list.add(a1);
                    list.add(a2);
                    list.add(a3);

                    if (list.size() != 3) return 1;
                    Int g1 = (Int)list.get(1);
                    if (g1.value() != 20) return 2;

                    Int a25 = new Int(25);
                    list.set(1, a25);
                    Int g1_2 = (Int)list.get(1);
                    if (g1_2.value() != 25) return 3;

                    Int a15 = new Int(15);
                    list.insert(1, a15);
                    // Now: [10, 15, 25, 30]
                    if (list.size() != 4) return 4;
                    Int g1_3 = (Int)list.get(1);
                    if (g1_3.value() != 15) return 5;
                    Int g2 = (Int)list.get(2);
                    if (g2.value() != 25) return 6;

                    if (list.index_of(a25) != 2) return 7;

                    Int removed = (Int)list.remove_at(1);
                    if (removed.value() != 15) return 8;
                    if (list.size() != 3) return 9;

                    bool remSuccess = list.remove(a25);
                    if (!remSuccess) return 10;
                    if (list.size() != 2) return 11;

                    String str = list.to_string();
                    if (!str.equals_chars("[10, 30]")) return 12;

                    return 0;
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 7.5: IDeque contract double-ended operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IDeque;
            import solix.collections.IIterator;
            import solix.collections.Collections;
            import solix.core.Int;
            import solix.core.IStringable;
            import solix.core.String;
            import solix.system.Console;

            class SimpleDeque implements IDeque {
                private IStringable[] data;
                private int32 head;
                private int32 tail;
                private int32 count;

                public SimpleDeque() {
                    this.data = new IStringable[20];
                    this.head = 10;
                    this.tail = 10;
                    this.count = 0;
                }

                public int32 size() { return this.count; }
                public bool is_empty() { return this.count == 0; }
                public void clear() { this.head = 10; this.tail = 10; this.count = 0; }

                public bool contains(IStringable item) {
                    for (int32 i = this.head; i < this.tail; i = i + 1) {
                        if (this.data[i] != null && this.data[i].to_string().equals(item.to_string())) return true;
                    }
                    return false;
                }

                public IStringable[] to_array() {
                    IStringable[] r = new IStringable[this.count];
                    for (int32 i = 0; i < this.count; i = i + 1) {
                        r[i] = this.data[this.head + i];
                    }
                    return r;
                }

                public IIterator iterator() {
                    return new DequeIterator(this.data, this.head, this.tail);
                }

                public String to_string() {
                    return Collections.to_string(this);
                }

                public void add_first(IStringable item) {
                    this.head = this.head - 1;
                    this.data[this.head] = item;
                    this.count = this.count + 1;
                }

                public void add_last(IStringable item) {
                    this.data[this.tail] = item;
                    this.tail = this.tail + 1;
                    this.count = this.count + 1;
                }

                public IStringable remove_first() {
                    IStringable v = this.data[this.head];
                    this.head = this.head + 1;
                    this.count = this.count - 1;
                    return v;
                }

                public IStringable remove_last() {
                    this.tail = this.tail - 1;
                    IStringable v = this.data[this.tail];
                    this.count = this.count - 1;
                    return v;
                }

                public IStringable peek_first() {
                    return this.data[this.head];
                }

                public IStringable peek_last() {
                    return this.data[this.tail - 1];
                }
            }

            class DequeIterator implements IIterator {
                private IStringable[] arr;
                private int32 cur;
                private int32 end;
                public DequeIterator(IStringable[] a, int32 h, int32 t) { this.arr = a; this.cur = h; this.end = t; }
                public bool has_next() { return this.cur < this.end; }
                public IStringable next() {
                    IStringable v = this.arr[this.cur];
                    this.cur = this.cur + 1;
                    return v;
                }
            }

            class Main {
                public static int32 main() {
                    IDeque deque = new SimpleDeque();
                    Int d10 = new Int(10);
                    Int d5 = new Int(5);
                    Int d20 = new Int(20);
                    deque.add_last(d10);
                    deque.add_first(d5);
                    deque.add_last(d20);
                    // Layout: [5, 10, 20]

                    if (deque.size() != 3) return 1;
                    Int p1 = (Int)deque.peek_first();
                    if (p1.value() != 5) return 2;
                    Int p2 = (Int)deque.peek_last();
                    if (p2.value() != 20) return 3;

                    String repr = deque.to_string();
                    if (!repr.equals_chars("[5, 10, 20]")) return 4;

                    Int f = (Int)deque.remove_first();
                    if (f.value() != 5) return 5;
                    if (deque.size() != 2) return 6;

                    Int l = (Int)deque.remove_last();
                    if (l.value() != 20) return 7;
                    if (deque.size() != 1) return 8;

                    return 0;
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 7.6: Negative: Calling next() on exhausted iterator throws NoSuchElementException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IIterator;
            import solix.exceptions.NoSuchElementException;
            import solix.core.Int;
            import solix.core.IStringable;

            class GuardedIterator implements IIterator {
                private int32 count;
                private int32 pos;

                public GuardedIterator(int32 c) {
                    this.count = c;
                    this.pos = 0;
                }

                public bool has_next() {
                    return this.pos < this.count;
                }

                public IStringable next() {
                    if (this.pos >= this.count) {
                        throw new NoSuchElementException("Iterator exhausted");
                    }
                    this.pos = this.pos + 1;
                    return new Int(this.pos);
                }
            }

            class Main {
                public static int32 main() {
                    IIterator it = new GuardedIterator(1);
                    it.next(); // pos = 1

                    try {
                        it.next(); // pos >= 1 -> throws!
                        return 1; // Should not reach here
                    } catch (NoSuchElementException e) {
                        return 0; // Expected
                    }
                }
            }
        )";

        
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
