#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
  RawMemory() = default;

  explicit RawMemory(size_t capacity)
	: buffer_(Allocate(capacity))
	, capacity_(capacity) 
  {
  }

  ~RawMemory() {
	Deallocate(buffer_);
  }

  RawMemory(const RawMemory&) = delete;
  RawMemory& operator=(const RawMemory& rhs) = delete;

  RawMemory(RawMemory&& other) noexcept
	: buffer_(nullptr)
	, capacity_(0)
  { 
	Swap(other);
  }

  RawMemory& operator=(RawMemory&& rhs) noexcept { 
	if (this !=  rhs) {
	  Swap(rhs);
	}
	return *this;
  }

  T* operator+(size_t offset) noexcept {
	assert(offset <= capacity_);
	return buffer_ + offset;
  }

  const T* operator+(size_t offset) const noexcept {
	return const_cast<RawMemory&>(*this) + offset;
  }

  const T& operator[](size_t index) const noexcept {
	return const_cast<RawMemory&>(*this)[index];
  }

  T& operator[](size_t index) noexcept {
	assert(index < capacity_);
	return buffer_[index];
  }

  void Swap(RawMemory& other) noexcept {
	std::swap(buffer_, other.buffer_);
	std::swap(capacity_, other.capacity_);
  }

  const T* GetAddress() const noexcept {
	return buffer_;
  }

  T* GetAddress() noexcept {
	return buffer_;
  }

  size_t Capacity() const {
	return capacity_;
  }

private:
// Выделяет сырую память под n элементов и возвращает указатель на неё
  static T* Allocate(size_t n) {
	return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
  }

// Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
  static void Deallocate(T* buf) noexcept {
	operator delete(buf);
  }

  T* buffer_ = nullptr;
  size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
  using iterator = T*;
  using const_iterator = const T*;

  Vector() = default;

  explicit Vector(size_t size);
  Vector(const Vector& other);
  Vector(Vector&& other) noexcept;

  ~Vector();

  Vector& operator=(const Vector& rhs);    
  Vector& operator=(Vector&& rhs) noexcept;

  iterator begin() noexcept {
	return data_.GetAddress();
  }

  iterator end() noexcept {
	return data_.GetAddress()+size_;
  }

  const_iterator begin() const noexcept {
	return data_.GetAddress();
  }

  const_iterator end() const noexcept {
	return data_.GetAddress()+size_;
  }

  const_iterator cbegin() const noexcept {
	return data_.GetAddress();
  }

  const_iterator cend() const noexcept {
	return data_.GetAddress()+size_;
  }

  const T& operator[](size_t index) const noexcept;

  T& operator[](size_t index) noexcept;

  void Swap(Vector& other) noexcept;

  void Reserve(size_t new_capacity);

  void Resize(size_t new_size);

  template <typename T1>
  void PushBack(T1&& value);

  void PopBack();

  template <typename... Args>
  T& EmplaceBack(Args&&... args);

  size_t Size() const noexcept;

  size_t Capacity() const noexcept;
  
  template <typename... Args>
  iterator Emplace(const_iterator pos, Args&&... args);

  iterator Erase(const_iterator pos);
  
  iterator Insert(const_iterator pos, const T& value);

  iterator Insert(const_iterator pos, T&& value);

private:
  RawMemory<T> data_;
  size_t size_ = 0;

// Вызывает деструктор объекта по адресу buf
  static void Destroy(T* buf) noexcept {
	buf->~T();
  }

// Вызывает деструкторы n объектов массива по адресу buf
  static void DestroyN(T* buf, size_t n) noexcept {
	for (size_t i = 0; i != n; ++i) {
	  Destroy(buf + i);
	}
  }

// Создаёт копию объекта elem в сырой памяти по адресу buf
  static void CopyConstruct(T* buf, const T& elem) {
	new (buf) T(elem);
  }

  template<typename InputIt, typename SizeT, typename NoThrowForwardIt >
  void UninitializedMoveOrCopy(InputIt from, SizeT size, NoThrowForwardIt to);

  template <typename... Args>
  void EmplaceWithRealoc(size_t offset, Args&&... args);

  template <typename... Args>
  void EmplaceWithoutRealoc(size_t offset, Args&&... args);
};

template <typename T>
Vector<T>::Vector(size_t size)
  : data_(size)
  , size_(size)
{
  std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template <typename T>
Vector<T>::Vector(const Vector& other)
  : data_(other.size_)
  , size_(other.size_)
{
  std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept 
  : data_()
  , size_()  //
{
  Swap(other);
}

template <typename T>
Vector<T>::~Vector() {
  DestroyN(data_.GetAddress(), size_);
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector<T>& rhs) {
  if (this != &rhs) {
	if (rhs.size_ > data_.Capacity()) {
	  Vector rhs_copy(rhs);
	  Swap(rhs_copy);
	} else {
	  if (rhs.size_ < size_) {
		std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
		std::destroy_n((data_+(rhs.size_)), size_-rhs.size_);
	  }
	  else {
		std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
		std::uninitialized_copy_n(rhs.data_+size_, rhs.size_-size_, data_+size_);
	  }
	  size_ =rhs.size_;
	}
  }
  return *this;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector<T>&& rhs) noexcept {
  if (this != &rhs) {
	Swap(rhs);
  }
  return *this;
}

template <typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
  return const_cast<Vector&>(*this)[index];
}

template <typename T>
T& Vector<T>::operator[](size_t index) noexcept {
  assert(index < size_);
  return data_[index];
}

template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
  std::swap(size_, other.size_);
  data_.Swap(other.data_);
}

template <typename T>
void Vector<T>::Reserve(size_t new_capacity) {
  if (new_capacity <= data_.Capacity()) {
	return;
  }

  RawMemory<T> new_data(new_capacity);       

  UninitializedMoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());

  std::destroy_n(data_.GetAddress(), size_);
  data_.Swap(new_data);
}

template <typename T>
void Vector<T>::Resize(size_t new_size) {
  if (new_size < size_) {
	std::destroy_n(data_+new_size, size_-new_size);
	size_ = new_size;
  }
  else if (new_size > size_) {
	Reserve(new_size);
	std::uninitialized_value_construct_n(data_+size_, new_size-size_);
	size_ = new_size;
  }
}

template <typename T>
template <typename T1>
void Vector<T>::PushBack(T1&& value) {
  if (size_ == Capacity()) {
	RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
	try {
	  new (new_data + size_) T(std::forward<T1>(value));
	}
	catch (...) {
	  throw;
	}

	UninitializedMoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());

	std::destroy_n(data_.GetAddress(), size_);
	data_.Swap(new_data);
  }
  else {
	new (data_ + size_) T(std::forward<T1>(value));
  }
  ++size_;
}

template <typename T>
void Vector<T>::PopBack() /* noexcept */ {
  std::destroy_n(data_ + size_ - 1, 1);
  --size_;	
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
  if (size_ < Capacity()) {
	new (data_ + size_) T(std::forward<Args>(args)...);
  }
  else { // capacity < size
	RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
	new (new_data + size_) T(std::forward<Args>(args)...);
	UninitializedMoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());

	std::destroy_n(data_.GetAddress(), size_);
	data_.Swap(new_data);
  }
  ++size_;
  return *(this->data_.GetAddress() + size_ - 1);
}

template <typename T>
size_t Vector<T>::Size() const noexcept {
  return size_;
}

template <typename T>
size_t Vector<T>::Capacity() const noexcept {
  return data_.Capacity();
}

template <typename T>
template <typename... Args>
T* Vector<T>::Emplace(const_iterator pos, Args&&... args) {
  size_t offset = pos - cbegin();
  if (size_ == Capacity()) {
	EmplaceWithRealoc(offset, std::forward<Args>(args)...);
  }
  else {
	EmplaceWithoutRealoc(offset, std::forward<Args>(args)...);
  }
  ++size_;
  return begin() + offset;
}

// using iterator = T*;
// Как вне тела класса воспользоваться iterator???
template <typename T>
T* Vector<T>::Erase(const_iterator pos) {
  size_t iter = pos - cbegin();
  size_t offset = pos - cbegin();
  if (pos != end()-1) {
	for (; iter < size_-1; ++iter) {
	  data_[iter] = std::move(data_[iter+1]);
	}
  }
  Destroy(&data_[iter]);
  --size_;
  return begin() + offset;
}

template <typename T>
T* Vector<T>::Insert(const_iterator pos, const T& value) {
  return Emplace(pos, value);
}

template <typename T>
T* Vector<T>::Insert(const_iterator pos, T&& value) {
  return Emplace(pos, std::move(value));
}

template <typename T>
template<typename InputIt, typename SizeT, typename NoThrowForwardIt >
void Vector<T>::UninitializedMoveOrCopy(InputIt from, SizeT size, NoThrowForwardIt to) {
  if constexpr (std::is_nothrow_move_constructible_v<T> ||  !std::is_copy_constructible_v<T>) {
	std::uninitialized_move_n(from, size, to);
  }
  else {
	std::uninitialized_copy_n(from, size, to);
  }
}

template <typename T>
template <typename... Args>
void Vector<T>::EmplaceWithRealoc(size_t offset, Args&&... args) {
  RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

  try {
	new (new_data.GetAddress() + offset) T(std::forward<Args>(args)...);
  }
  catch (...) {
	throw;
  }

  try {
	UninitializedMoveOrCopy(data_.GetAddress(), offset, new_data.GetAddress());
  }
  catch (...) {
	Destroy(new_data + offset);
	throw;
  }

  try {
	UninitializedMoveOrCopy(data_.GetAddress()+offset, size_ - offset, new_data.GetAddress() + offset + 1);
  }
  catch (...) {
	std::destroy_n(new_data.GetAddress(),  offset);
  }

  std::destroy_n(data_.GetAddress(), size_);
  data_.Swap(new_data);
}

template <typename T>
template <typename... Args>
void Vector<T>::EmplaceWithoutRealoc(size_t offset, Args&&... args) {
  T new_data = T(std::forward<Args>(args)...);
  new(end()) T(std::forward<T>(*(end()-1)));
  std::move_backward(begin() + offset, end() - 1, end());
  *(begin() + offset) = std::move(new_data); 
}
