#pragma once

#undef __out
#undef __in

#include <functional>
#include <atomic>
#include <tuple>
#include <type_traits>

#include <boost/system/error_code.hpp>

namespace detail
{
    template <typename F, typename Tuple, std::size_t ... I>
    constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>){
        return static_cast <F&&>(f)(std::get <I>(static_cast <Tuple&&>(t))...);
    }

    template <class F, class Tuple>
    constexpr decltype(auto) apply(F&& f, Tuple&& t)
    {
        return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
            std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
    }

    template <typename F, typename Tuple, std::size_t ... I>
    constexpr decltype(auto) apply_non_deduce_impl(F const& f, Tuple const& t, std::index_sequence<I...>){
         return f(std::get <I>(t)...);
    }

    template <class F, class Tuple>
    constexpr decltype(auto) apply_non_deduce(F const& f, Tuple const& t)
    {
        return apply_non_deduce_impl(f, t,
            std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
    }
}

/**
 *  This class represents a promise encapsulating
 *  a success callback and a fail callback.
 */
class callback_wrapper
{
public:
    friend void swap(callback_wrapper& first, callback_wrapper& second)
    {
        using std::swap;

        swap(first.func_, second.func_);
        swap(first.fail_, second.fail_);
        first.fullfilled_.exchange(second.fullfilled_.exchange(first.fullfilled_.load()));
        first.error_.exchange(second.error_.exchange(first.error_.load()));
    }

    callback_wrapper()
        : func_{}
        , fail_{}
        , fullfilled_{false}
        , error_{false}
    {
    }

    ~callback_wrapper() = default;

    /**
     *  Make callback wrappers copyable.
     */
    callback_wrapper(callback_wrapper const& other)
        : func_{other.func_}
        , fail_{other.fail_}
        , fullfilled_{other.fullfilled_.load()}
        , error_{other.error_.load()}
    {
    }

    /**
     *  Make callback wrappers movable
     */
    callback_wrapper(callback_wrapper&& other)
        : func_{std::move(other.func_)}
        , fail_{std::move(other.fail_)}
        , fullfilled_{other.fullfilled_.load()}
        , error_{other.error_.load()}
    {
    }

    /**
     *  Assignment
     */
    callback_wrapper& operator=(callback_wrapper const& other)
    {
        auto cpy = other;
        swap(*this, cpy);
        return *this;
    }

    callback_wrapper& operator=(callback_wrapper&& other) noexcept
    {
        // all statements in this functions are noexcept
        func_ = std::move(other.func_); // noexcept
        fail_ = std::move(other.fail_); // noexcept
        fullfilled_.store(other.fullfilled_.load()); // load&store are noexcept
        error_.store(other.error_.load()); // load&store are noexcept
        return *this;
    }

    /**
     *  Sets a continuation function.
     */
    template <typename FunctionT>
    callback_wrapper& then(FunctionT const& f)
    {
        if (fullfilled_.load())
            f();
        else
            func_ = f;

        return *this;
    }

    /**
     *  Sets a continuation function.
     */
    template <typename FunctionT>
    callback_wrapper& then(FunctionT&& f)
    {
        if (fullfilled_.load())
            f();
        else
            func_ = std::move(f);

        return *this;
    }

    /**
     *  Sets an error callback.
     */
    template <typename FunctionT>
    callback_wrapper& except(FunctionT const& f)
    {
        if (error_.load())
            f(last_ec_.load());
        else
            fail_ = f;

        return *this;
    }

    /**
     *  Sets an error callback.
     */
    template <typename FunctionT>
    callback_wrapper& except(FunctionT&& f)
    {
        if (error_.load())
            f(last_ec_.load());
        else
            fail_ = std::move(f);

        return *this;
    }

    /**
     *  Fullfill the "promise-like" callback wrapper.
     *  Will either call func_ or set a flag, that
     *  then calls the function immediately.
     */
    callback_wrapper& fullfill()
    {
        fullfilled_.store(true);

        if (func_)
            func_();

        return *this;
    }

    /**
     *  Resets the then() and except() callback
     */
    callback_wrapper& reset()
    {
        func_ = {};
        error_ = {};
        return *this;
    }

    /**
     *  Same as fullfill, but instead of calling the continuation function,
     *  it will call the error handler function with the ec.
     */
    callback_wrapper& error(boost::system::error_code const& ec)
    {
        if (fail_)
            fail_(ec);
        else
            last_ec_.store(ec);

        error_.store(true);
        return *this;
    }

private:
    std::function <void()> func_;
    std::function <void(boost::system::error_code)> fail_;
    std::atomic_bool fullfilled_;
    std::atomic_bool error_;
    std::atomic <boost::system::error_code> last_ec_;
};

class callback_passthrough
{
public:
    template <typename T>
    callback_passthrough& then(T&& f)
    {
        f();
        return *this;
    }
};

class final_callback
{
public:
    template <typename T>
    final_callback& then(T&&)
    {
        // end it here
        return *this;
    }
};

using promise = callback_wrapper;
