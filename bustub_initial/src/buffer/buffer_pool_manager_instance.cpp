//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/exception.h"
#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  page_table_ = new ExtendibleHashTable<page_id_t, frame_id_t>(bucket_size_);
  replacer_ = new LRUKReplacer(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
}

auto BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);

  frame_id_t frame_id = -1;

  // 1. 尝试从 free list 获取空闲帧
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
  } else {
    // 2. 如果没有空闲帧，从 replacer 中驱逐一个帧
    if (!replacer_->Evict(&frame_id)) {
      // 所有帧都被 pin 了，无法分配新页
      return nullptr;
    }

    // 3. 如果被驱逐的帧是脏页，写回磁盘
    Page *victim_page = &pages_[frame_id];
    if (victim_page->IsDirty()) {
      disk_manager_->WritePage(victim_page->GetPageId(), victim_page->GetData());
      victim_page->is_dirty_ = false;
    }

    // 4. 从页表中删除旧页的映射
    page_table_->Remove(victim_page->GetPageId());
  }

  // 5. 分配新的页 ID
  *page_id = AllocatePage();

  // 6. 重置页的元数据和内存
  Page *new_page = &pages_[frame_id];
  new_page->page_id_ = *page_id;
  new_page->pin_count_ = 1;
  new_page->is_dirty_ = false;
  new_page->ResetMemory();

  // 7. 将新页添加到页表
  page_table_->Insert(*page_id, frame_id);

  // 8. 记录访问历史并设置为不可驱逐
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);

  return new_page;
}

auto BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);

  // 1. 在页表中查找页
  frame_id_t frame_id;
  if (page_table_->Find(page_id, frame_id)) {
    // 页已在缓冲池中
    Page *page = &pages_[frame_id];
    page->pin_count_++;

    // 记录访问并设置为不可驱逐
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);

    return page;
  }

  // 2. 页不在缓冲池中，需要从磁盘读取
  // 2.1 获取空闲帧
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
  } else {
    // 2.2 从 replacer 中驱逐一个帧
    if (!replacer_->Evict(&frame_id)) {
      // 所有帧都被 pin 了，无法获取页
      return nullptr;
    }

    // 2.3 如果被驱逐的帧是脏页，写回磁盘
    Page *victim_page = &pages_[frame_id];
    if (victim_page->IsDirty()) {
      disk_manager_->WritePage(victim_page->GetPageId(), victim_page->GetData());
      victim_page->is_dirty_ = false;
    }

    // 2.4 从页表中删除旧页的映射
    page_table_->Remove(victim_page->GetPageId());
  }

  // 3. 从磁盘读取页数据
  Page *page = &pages_[frame_id];
  disk_manager_->ReadPage(page_id, page->GetData());

  // 4. 更新页的元数据
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page->is_dirty_ = false;

  // 5. 将页添加到页表
  page_table_->Insert(page_id, frame_id);

  // 6. 记录访问历史并设置为不可驱逐
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);

  return page;
}

auto BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  // 1. 在页表中查找页
  frame_id_t frame_id;
  if (!page_table_->Find(page_id, frame_id)) {
    // 页不在缓冲池中
    return false;
  }

  Page *page = &pages_[frame_id];

  // 2. 检查 pin count
  if (page->pin_count_ <= 0) {
    // pin count 已经为 0
    return false;
  }

  // 3. 减少 pin count
  page->pin_count_--;

  // 4. 更新脏标记
  if (is_dirty) {
    page->is_dirty_ = true;
  }

  // 5. 如果 pin count 为 0，设置为可驱逐
  if (page->pin_count_ == 0) {
    replacer_->SetEvictable(frame_id, true);
  }

  return true;
}

auto BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  // 1. 在页表中查找页
  frame_id_t frame_id;
  if (!page_table_->Find(page_id, frame_id)) {
    // 页不在缓冲池中
    return false;
  }

  // 2. 将页写回磁盘（无论是否为脏页）
  Page *page = &pages_[frame_id];
  disk_manager_->WritePage(page_id, page->GetData());

  // 3. 清除脏标记
  page->is_dirty_ = false;

  return true;
}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  std::lock_guard<std::mutex> lock(latch_);

  // 遍历所有页并将它们写回磁盘
  for (size_t i = 0; i < pool_size_; i++) {
    Page *page = &pages_[i];
    if (page->page_id_ != INVALID_PAGE_ID) {
      disk_manager_->WritePage(page->page_id_, page->GetData());
      page->is_dirty_ = false;
    }
  }
}

auto BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  // 1. 在页表中查找页
  frame_id_t frame_id;
  if (!page_table_->Find(page_id, frame_id)) {
    // 页不在缓冲池中，视为成功
    return true;
  }

  Page *page = &pages_[frame_id];

  // 2. 检查页是否被 pin
  if (page->pin_count_ > 0) {
    // 页仍被使用，无法删除
    return false;
  }

  // 3. 如果是脏页，写回磁盘
  if (page->IsDirty()) {
    disk_manager_->WritePage(page->GetPageId(), page->GetData());
  }

  // 4. 从页表中删除页
  page_table_->Remove(page_id);

  // 5. 停止在 replacer 中跟踪该帧
  replacer_->Remove(frame_id);

  // 6. 将帧添加回 free list
  free_list_.push_back(frame_id);

  // 7. 重置页的元数据和内存
  page->page_id_ = INVALID_PAGE_ID;
  page->pin_count_ = 0;
  page->is_dirty_ = false;
  page->ResetMemory();

  // 8. 调用 DeallocatePage 模拟释放磁盘上的页
  DeallocatePage(page_id);

  return true;
}

auto BufferPoolManagerInstance::AllocatePage() -> page_id_t { return next_page_id_++; }

}  // namespace bustub

// 实现总结
// 1. NewPgImp() - 创建新页

// 从 free list 或 replacer 获取空闲帧
// 如果驱逐的帧是脏页，先写回磁盘
// 分配新的页 ID
// 初始化页的元数据（pin_count=1, is_dirty=false）
// 加入页表映射
// 设置为不可驱逐并记录访问
// 2. FetchPgImp() - 获取页

// 先在页表中查找（缓冲池命中）
// 如果命中：增加 pin count，记录访问
// 如果未命中：
// 获取空闲帧或驱逐帧
// 从磁盘读取页数据
// 更新页表和元数据
// 设置为不可驱逐

// 3. UnpinPgImp() - 释放页

// 减少 pin count
// 更新脏标记
// 当 pin count 降为 0 时，设置为可驱逐
// 4. FlushPgImp() - 刷新页到磁盘

// 将指定页写回磁盘
// 清除脏标记
// 5. FlushAllPgsImp() - 刷新所有页

// 遍历所有有效页并写回磁盘
// 6. DeletePgImp() - 删除页

// 检查页是否被 pin（被 pin 则无法删除）
// 如果是脏页，先写回磁盘
// 从页表删除映射
// 从 replacer 移除跟踪
// 将帧归还到 free list
// 重置页的元数据
