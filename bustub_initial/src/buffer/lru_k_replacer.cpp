//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);

  if (curr_size_ == 0) {
    return false;
  }

  // 查找具有最大后退 k-距离的可淘汰帧
  frame_id_t victim_frame = -1;
  size_t max_backward_k_dist = 0;
  size_t earliest_timestamp = std::numeric_limits<size_t>::max();

  for (auto &[fid, entry] : frame_map_) {
    if (!entry.is_evictable_) {
      continue;
    }

    size_t backward_k_dist;
    size_t frame_earliest_ts;

    if (entry.history_.size() < k_) {
      // 访问次数少于 k，后退 k-距离为 +inf
      backward_k_dist = std::numeric_limits<size_t>::max();
      // 使用最早的访问时间戳
      frame_earliest_ts = entry.history_.back();
    } else {
      // 计算后退 k-距离：当前时间戳 - 第k次访问的时间戳
      auto it = entry.history_.begin();
      std::advance(it, k_ - 1);
      backward_k_dist = current_timestamp_ - *it;
      frame_earliest_ts = entry.history_.back();
    }

    // 选择后退 k-距离最大的帧
    // 如果距离相同，选择最早时间戳的帧
    if (victim_frame == -1 || backward_k_dist > max_backward_k_dist ||
        (backward_k_dist == max_backward_k_dist && frame_earliest_ts < earliest_timestamp)) {
      victim_frame = fid;
      max_backward_k_dist = backward_k_dist;
      earliest_timestamp = frame_earliest_ts;
    }
  }

  if (victim_frame == -1) {
    return false;
  }

  // 淘汰该帧
  *frame_id = victim_frame;
  frame_map_.erase(victim_frame);
  curr_size_--;

  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);

  if (frame_id < 0 || static_cast<size_t>(frame_id) >= replacer_size_) {
    throw Exception("Invalid frame_id");
  }

  current_timestamp_++;

  // 如果帧不存在，创建新条目
  if (frame_map_.find(frame_id) == frame_map_.end()) {
    frame_map_[frame_id] = FrameEntry();
  }

  // 在历史记录前面添加当前时间戳
  frame_map_[frame_id].history_.push_front(current_timestamp_);
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::scoped_lock<std::mutex> lock(latch_);

  if (frame_id < 0 || static_cast<size_t>(frame_id) >= replacer_size_) {
    throw Exception("Invalid frame_id");
  }

  // 如果帧不存在，直接返回
  if (frame_map_.find(frame_id) == frame_map_.end()) {
    return;
  }

  auto &entry = frame_map_[frame_id];

  // 更新可淘汰状态并调整大小
  if (entry.is_evictable_ && !set_evictable) {
    // 从可淘汰变为不可淘汰
    entry.is_evictable_ = false;
    curr_size_--;
  } else if (!entry.is_evictable_ && set_evictable) {
    // 从不可淘汰变为可淘汰
    entry.is_evictable_ = true;
    curr_size_++;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);

  // 如果帧不存在，直接返回
  if (frame_map_.find(frame_id) == frame_map_.end()) {
    return;
  }

  auto &entry = frame_map_[frame_id];

  // 如果帧不可淘汰，抛出异常
  if (!entry.is_evictable_) {
    throw Exception("Cannot remove non-evictable frame");
  }

  // 移除帧并减少大小
  frame_map_.erase(frame_id);
  curr_size_--;
}

auto LRUKReplacer::Size() -> size_t {
  std::scoped_lock<std::mutex> lock(latch_);
  return curr_size_;
}

}  // namespace bustub

// 数据结构
// FrameEntry: 记录每个帧的访问历史和可淘汰状态
// history_: 访问时间戳列表（最新在前）
// is_evictable_: 是否可淘汰标志
// frame_map_: 帧ID到FrameEntry的映射
// current_timestamp_: 当前时间戳计数器
// curr_size_: 当前可淘汰帧的数量

// 核心算法
// 1. Evict() - 淘汰算法

// 遍历所有可淘汰的帧
// 计算每个帧的后退 k-距离：
// 如果访问次数 < k：距离 = +∞（使用最早时间戳比较）
// 如果访问次数 ≥ k：距离 = 当前时间戳 - 第k次访问的时间戳
// 选择距离最大的帧淘汰
// 相同距离时，选择最早时间戳的帧
// 2. RecordAccess() - 记录访问

// 增加时间戳计数器
// 将新时间戳添加到帧的访问历史前面
// 如果帧不存在，创建新条目
// 3. SetEvictable() - 设置可淘汰状态

// 更新帧的可淘汰标志
// 动态调整 curr_size_（可淘汰帧数）
// 4. Remove() - 移除帧

// 检查帧是否可淘汰（不可淘汰则抛异常）
// 从映射中删除帧
// 减少 curr_size_
// 5. Size() - 返回大小

// 返回当前可淘汰帧的数量
// 线程安全
// 所有公共方法都使用 std::scoped_lock<std::mutex> 保护共享数据