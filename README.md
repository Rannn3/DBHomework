# SCU 数据库实验课程项目
📋 项目说明

本项目基于 CMU 15-445/645 数据库系统课程的 BusTub 项目进行开发，实现了数据库系统的核心组件。通过完成该项目，深入理解了数据库存储管理、索引结构和并发控制等关键技术。

---

# 🛠️ 已实现功能

## Project 1: 缓冲池管理器 (2025.11.08)

### ✓ Extendible Hash Table
- 可扩展哈希表，支持动态扩展和线程安全访问

### ✓ LRU-K Replacer
- 基于访问历史的智能页面替换算法

### ✓ Buffer Pool Manager
- 完整的缓冲池管理器，处理内存与磁盘间的页面交换

---

## Project 2: B+树索引 (2025.11.27)

### ✓ B+Tree Pages
- B+树叶子页和内部页的完整实现

### ✓ B+Tree Insert
- 插入操作，支持页面自动分裂

### ✓ B+Tree Delete
- 删除操作，支持合并和重分布

### ✓ Index Iterator
- 索引迭代器，支持高效的范围扫描

### ✓ Concurrent B+Tree
- 并发 B+树，基于 Latch Crabbing 协议

---

# 🧪 测试指南

## 环境准备
确保系统已安装必要开发工具：
- CMake ≥ 3.5
- 支持 C++17 的编译器
- GNU Make 或 Ninja

---

# 📦 项目编译

```bash
cd bustub_initial
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)     # macOS
# 或
make -j$(nproc)                 # Linux

## 🧪 功能测试

### 1. 测试可扩展哈希表

**相关文件：**
- `src/container/hash/extendible_hash_table.cpp`
- `src/include/container/hash/extendible_hash_table.h`

**测试步骤：**
```bash
cd bustub_initial/build
make extendible_hash_table_test
./test/extendible_hash_table_test
## 2. 测试 B+ 树索引

**相关文件：**

- `src/storage/index/b_plus_tree.cpp`
- `src/storage/index/index_iterator.cpp`
- `src/storage/page/b_plus_tree_leaf_page.cpp`
- `src/storage/page/b_plus_tree_internal_page.cpp`

**测试插入功能：**
```bash
make b_plus_tree_insert_test
./test/b_plus_tree_insert_test