#ifndef PROTOCOL_UTILS_FUNC_SET_HPP
#define PROTOCOL_UTILS_FUNC_SET_HPP

#include "../common.hpp"

#include <type_traits>

namespace protocol
{

template <class T>
struct GetReturnType;

template <typename TRet, typename... TArgs>
struct GetReturnType<TRet (*)(TArgs...)>
{
    using type = TRet;
};

template <typename TRet, typename... TArgs>
using FuncPtr = TRet (*)(TArgs...);

template <typename TRet, typename... TArgs>
using FuncConstPtr = TRet (*const)(TArgs...);

template <typename TRet, typename... TArgs>
using FuncInstance = FuncPtr<TRet, void*, TArgs...>;

struct FuncSetTag {};

template <typename TInstance, typename TRet, typename... TArgs>
struct FuncSet : public FuncSetTag
{
    using InstanceType = TInstance;
    // using ReturnType = typename GetReturnType<decltype(c_func)>::type;
    using ReturnType = TRet;

    using FuncType = FuncPtr<ReturnType, InstanceType*, TArgs...>;

    // static_assert(std::is_same<decltype(c_func), FuncPtr<ReturnType, TInstance*, TArgs...>>::value, 
    //               "c_func must be a function pointer of type FuncPtr<ReturnType, TInstance*, TArgs...>");

    explicit constexpr FuncSet(InstanceType* instance, FuncType func) : instance(instance), func_ptr(func) {}

    explicit constexpr FuncSet() : instance(nullptr), func_ptr(nullptr) {}
    
    explicit constexpr FuncSet(const FuncSet& other) : instance(other.instance), func_ptr(other.func_ptr) {}
    FuncSet& operator=(const FuncSet& other) {
        instance = other.instance;
        func_ptr = other.func_ptr;
        return *this;
    }
    
    explicit constexpr FuncSet(FuncSet&& other) : instance(other.instance), func_ptr(other.func_ptr) {}
    FuncSet& operator=(FuncSet&& other) {
        instance = other.instance;
        func_ptr = other.func_ptr;
        return *this;
    }

    __attribute__((always_inline))
    ReturnType callFunction(TArgs... args) {
        return func_ptr(instance, args...);
    }

    bool isOk() const {
        return instance != nullptr && func_ptr != nullptr;
    }

    auto getInstance() const {
        return instance;
    }

    auto getFunction() const {
        return func_ptr;
    }

private:
    InstanceType* instance;
    FuncPtr<ReturnType, InstanceType*, TArgs...> func_ptr;
};

struct StaticFuncSetTag {};

template <auto c_func, auto c_instance, typename... TArgs>
struct StaticFuncSet : public StaticFuncSetTag
{
    using InstanceType = typename std::remove_pointer_t<decltype(c_instance)>;
    using ReturnType = typename GetReturnType<decltype(c_func)>::type;

    static_assert(std::is_pointer<decltype(c_instance)>::value, "c_instance must be a pointer");
    static_assert(std::is_same<decltype(c_func), FuncPtr<ReturnType, TArgs...>>::value ||
                  std::is_same<decltype(c_func), FuncConstPtr<ReturnType, TArgs...>>::value, 
                  "c_func must be a function pointer of type FuncPtr<ReturnType, TArgs...>");

    explicit constexpr StaticFuncSet() {}

    static constexpr inline InstanceType* const instance = c_instance;

    __attribute__((always_inline))
    static constexpr inline ReturnType callFunction(TArgs... args) {
        return c_func(args...);
    };
};

} // namespace protocol

#endif // PROTOCOL_UTILS_FUNC_SET_HPP