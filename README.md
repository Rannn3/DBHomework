SCU 数据库实验课程项目
📋 项目说明

本项目基于 CMU 15-445/645 数据库系统课程的 BusTub 项目进行开发，实现了数据库系统的核心组件。通过完成该项目，深入理解了数据库存储管理、索引结构和并发控制等关键技术。

🛠️ 已实现功能
Project 1: 缓冲池管理器 (2025.11.08)
✅ Extendible Hash Table - 可扩展哈希表，支持动态扩展和线程安全访问

✅ LRU-K Replacer - 基于访问历史的智能页面替换算法

✅ Buffer Pool Manager - 完整的缓冲池管理器，处理内存与磁盘间的页面交换

Project 2: B+树索引 (2025.11.27)
✅ B+Tree Pages - B+树叶子页和内部页的完整实现

✅ B+Tree Insert - 插入操作，支持页面自动分裂

✅ B+Tree Delete - 删除操作，支持合并和重分布

✅ Index Iterator - 索引迭代器，支持高效的范围扫描

✅ Concurrent B+Tree - 并发B+树，基于Latch Crabbing协议

🧪 测试指南
环境准备
确保系统已安装必要的开发工具：

CMake (版本 3.5 或更高)

支持 C++17 标准的编译器

GNU Make 或 Ninja

📦 项目编译
bash
cd bustub_initial
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)  # macOS 系统
# 或 make -j$(nproc)        # Linux 系统
🧪 功能测试
测试可扩展哈希表
功能说明：实现了一个无需预先指定大小的可扩展哈希表，用于管理缓冲池中页面ID和帧ID的映射关系。

相关文件：

实现文件：src/container/hash/extendible_hash_table.cpp

头文件：src/include/container/hash/extendible_hash_table.h

测试步骤：

bash
cd bustub_initial/build
make extendible_hash_table_test
./test/extendible_hash_table_test
测试 B+树索引
功能说明：实现了支持并发访问的B+树索引，包含完整的插入、删除、查找和范围扫描功能。

相关文件：

B+树核心实现：src/storage/index/b_plus_tree.cpp

迭代器实现：src/storage/index/index_iterator.cpp

叶子页面：src/storage/page/b_plus_tree_leaf_page.cpp

内部页面：src/storage/page/b_plus_tree_internal_page.cpp

测试步骤：

bash
cd bustub_initial/build

# 测试插入功能
make b_plus_tree_insert_test
./test/b_plus_tree_insert_test

# 测试删除功能
make b_plus_tree_delete_test
./test/b_plus_tree_delete_test --gtest_also-run_disabled_tests

# 测试并发功能
make b_plus_tree_concurrent_test
./test/b_plus_tree_concurrent_test
🎯 完整测试套件
bash
# 编译所有测试用例
make build-tests

# 运行完整测试套件
make check-tests
🐛 调试技巧
遇到测试失败时，可以采取以下调试策略：

查看详细错误信息：

bash
./test/extendible_hash_table_test --gtest_filter=ExtendibleHashTableTest.InsertSplit
内存问题检测（Debug模式已自动启用Address Sanitizer）

查看测试日志：

bash
cat Testing/Temporary/LastTest.log
📊 代码质量
bash
# 自动格式化代码
make format

# 代码风格检查
make check-lint
📝 实现技术要点
Project 1: 缓冲池管理器
Extendible Hash Table
支持动态扩展，无需预设容量

线程安全设计，使用互斥锁保护关键区域

实现桶分裂和目录扩展机制

LRU-K Replacer
基于K次访问历史的页面使用追踪

淘汰后退k-距离最大的页面

完整的并发访问支持

Buffer Pool Manager
管理内存页面与磁盘页面的数据交换

实现页面固定和释放机制

自动处理脏页写回操作

集成LRU-K页面替换策略

Project 2: B+树索引
B+Tree Pages 实现
Leaf Page: 存储实际键值对，通过next_page_id形成有序链表

Internal Page: 存储导航键和子页面指针

相关文件：

src/storage/page/b_plus_tree_page.cpp - 基类实现

src/storage/page/b_plus_tree_leaf_page.cpp - 叶子页面

src/storage/page/b_plus_tree_internal_page.cpp - 内部页面

B+Tree 插入操作
从根节点向下遍历定位目标叶子节点

在叶子节点插入键值对

节点满载时触发自动分裂

向上递归处理分裂产生的新键

B+Tree 删除操作
定位并删除目标键值对

节点过小时触发合并或重分布

向上递归更新内部节点结构

Index Iterator
实现Begin(), Begin(key), End()等方法

支持operator*, operator++, operator==, operator!=

使用移动语义优化资源管理

正确管理页面的pin/unpin操作

Concurrent B+Tree (Latch Crabbing协议)
采用先进的锁抓取技术实现线程安全：

Search 操作：

获取父节点读锁 → 获取子节点读锁 → 释放父节点读锁

重复直至定位叶子节点

Insert/Delete 操作：

获取父节点写锁 → 获取子节点写锁

若子节点"安全"，释放所有祖先锁

安全节点判断标准：

Insert: size < max_size - 1（插入后不会满）

Delete: size > min_size（删除后不会过小）