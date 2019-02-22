#pragma once

struct MyTestSt1 {
  MyTestSt1(const std::string& ax, int by) :a(ax), b(by) {}
  std::string a;
  int b;
};

template<typename T1>
struct MyTestSt2 {
  MyTestSt2(const T1& ax) :a(ax) {}
  T1 a;
};

template<typename T1, typename T2>
struct MyTestSt3 {
  MyTestSt3(const T1& ax, T2 by);
  T1 a;
  T2 b;
};

template<typename T1, typename T2>
MyTestSt3<T1, T2>::MyTestSt3(const T1& ax, T2 by)
  :a(ax) ,b(by){
};

template<typename T1, int bb = 10>
struct MyTestSt {
  MyTestSt(T1& ax);
  T1 a;
  int b;
};

template<typename T1, int bb>
MyTestSt<T1, bb>::MyTestSt(T1& ax)
  :a(ax) {
  b = bb;
};
