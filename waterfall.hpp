#pragma once

#include "promise.hpp"

namespace PromiseWaterfall
{
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

    template <typename T, typename... List>
    inline void waterfall(T&& front, List&&... list);

    inline void waterfall()
    {
    }

    /**
     *  Instead of passing several parameters to the interjection method, pass this struct.
     */
    template <typename ReturnT>
    struct interjection_context
    {
        using ReturnType = ReturnT;

        /// The amount of times the interjection was called
        int count;

        /**
         *  The interjection function that is used.
         *  Provided so it can be changed on the fly.
         */
        std::function <ReturnT(interjection_context<ReturnT>&)> interjection;
    };

    template <typename ReturnT, typename T, typename... List>
    inline void waterfall_interject(std::function <ReturnT(interjection_context<ReturnT>&)> const& interjection, T&& front, List&&... list);

    template <typename ReturnT>
    inline void waterfall_interject(std::function <ReturnT(interjection_context<ReturnT>&)> const& interjection)
    {
    }

    namespace detail
    {
        template <typename ReturnT, typename T, typename... List>
        void waterfall_interject_impl(interjection_context <ReturnT> interjectionContext, T&& front, List&&... list)
        {
            front().then
            (
                [
                    nextParameters = std::make_tuple(
                        interjectionContext,
                        std::forward <List>(list)...
                    )
                ]()
                {
                    auto interjectionContext = std::get <0>(nextParameters);

                    if constexpr (sizeof...(List) >= 1)
                    {
                        if constexpr (std::is_same <ReturnT, bool>::value)
                        {
                            if (!interjectionContext.interjection(interjectionContext))
                                return;
                        }
                        else
                            interjectionContext.interjection(interjectionContext);
                        interjectionContext.count++;
                        detail::apply_non_deduce(&waterfall_interject_impl <ReturnT, std::decay_t <List> const&...>, nextParameters);
                    }
                }
            );
        }

        template <typename ReturnT, typename IteratorT>
        inline void waterfall_interject_iter_impl(
            interjection_context <ReturnT> interjectionContext,
            IteratorT begin,
            IteratorT end
        )
        {
            if (begin == end)
                return;
            (*begin)().then
            (
                [begin, end, &interjectionContext]()
                {
                    if (begin + 1 != end)
                    {
                        if constexpr (std::is_same <ReturnT, bool>::value)
                        {
                            if (!interjectionContext.interjection(interjectionContext))
                                return;
                        }
                        else
                            interjectionContext.interjection(interjectionContext);
                        interjectionContext.count++;
                        waterfall_interject_iter_impl(interjectionContext, begin + 1, end);
                    }
                }
            );
        }
    }

    template <typename T, typename... List>
    inline void waterfall(T&& front, List&&... list)
    {
        front().then([list = std::make_tuple(std::forward<List>(list)...)](){
            if constexpr (sizeof...(List) >= 1)
                detail::apply_non_deduce(&waterfall <std::decay_t <List> const&...>, list);
        });
    }

    template <typename ReturnT, typename T, typename... List>
    inline void waterfall_interject(std::function <ReturnT(interjection_context<ReturnT>&)> const& interjection, T&& front, List&&... list)
    {
        interjection_context <ReturnT> ctx{};
        ctx.interjection = interjection;
        detail::waterfall_interject_impl(std::move(ctx), std::forward <T>(front), std::forward <List>(list)...);
    }

    template <typename ReturnT, typename IteratorT>
    inline void waterfall_interject(
        IteratorT begin,
        IteratorT end,
        std::function <ReturnT(interjection_context<ReturnT>&)> const& interjection
    )
    {
        interjection_context <ReturnT> ctx{};
        ctx.interjection = interjection;

        detail::waterfall_interject_iter_impl(ctx, begin, end);
    }

    template <typename T, typename... List>
    inline void waterfall_interject(std::function <bool(interjection_context<bool>&)> const& interjection, T&& front, List&&... list)
    {
        waterfall_interject<bool>(interjection, std::forward <T&&>(front), std::forward <List&&>(list)...);
    }

    template <typename T, typename... List>
    inline void waterfall_interject(std::function <void(interjection_context<void>&)> const& interjection, T&& front, List&&... list)
    {
        waterfall_interject<void>(interjection, std::forward <T&&>(front), std::forward <List&&>(list)...);
    }
}
