#pragma once

#include <exception>

class BadWeakPtr : public std::exception {};

template <typename T, bool is_array = false>
class SharedPtr;

template <typename T, bool is_array = false>
class WeakPtr;


template <typename T>
class EnableSharedFromThis;