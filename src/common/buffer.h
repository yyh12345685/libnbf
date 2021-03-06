#pragma once

#include <stdlib.h>
#include <string.h>

namespace bdf {

class Buffer{
public:
  Buffer() : 
    pos_begin_(0),
    pos_end_(0), 
    pos_reading_(0), 
    pos_writing_(0) {
  }

  ~Buffer(){
    this->DestroyAll();
  }

  inline bool Write(const void *data, size_t length){
    if (pos_writing_ + length <= pos_end_ || EnsureSize(length)){
      memcpy(pos_writing_, data, length);
      pos_writing_ += length;
      return true;
    }
    return false;
  }

  inline char *GetReading() const{
    return pos_reading_;
  }

  inline size_t GetReadingSize() const {
    return (pos_writing_ - pos_reading_);
  }

  inline char *GetWriting() const{
    return pos_writing_;
  }

  inline size_t GetWritingSize() const{
    return (pos_end_ - pos_writing_);
  }

  inline size_t GetCapacity() const{
    return (pos_end_ - pos_begin_);
  }

  /// Move the data-reading cursor
  inline void DrainReading(size_t len){
    pos_reading_ += len;
    if (pos_reading_ >= pos_writing_) ResetAll();
  }

  /// Move the data-writing cursor
  inline bool PourWriting(size_t len){
    if (pos_end_ >= pos_writing_ + len){
      pos_writing_ += len;
      return true;
    }
    return false;
  }

  /// Roll back the poured
  inline bool StripWriting(size_t len){
    if (pos_writing_ >= pos_reading_ + len){
      pos_writing_ -= len;
      return true;
    }
    return false;
  }

  /// Reset pointers
  inline void ResetAll(){
    pos_reading_ = pos_writing_ = pos_begin_;
  }

  /// Release memory allocated
  void DestroyAll();

  /// Compact spaces within the limit if possible (eg. 2048)
  bool ShrinkSpace(size_t max_size);

  /// Ensure (expand) the size of free space
  bool EnsureSize(size_t need);

  /// The binary-safe strstr()!
  int FindBytes(const void *pattern, size_t len, int from=0);

 private:
  char *pos_begin_;
  char *pos_end_;
  char *pos_reading_;
  char *pos_writing_;
};

}

