#pragma once

#include "promise.hpp"

namespace PromiseWaterfall
{
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
         *  The previous promise that was handled. Can never be nullptr, since the interjection is not called before any
         *  promise was executed.
         */
        callback_wrapper* previous;

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
        void waterfall_interject_impl(interjection_context <ReturnT>* interjectionContext, T&& front, List&&... list)
        {
            auto&& prev = front();
            prev.then(
                [
                    &prev,
                    nextParameters = std::make_tuple(
                        interjectionContext,
                        std::forward <List>(list)...
                    )
                ](){
                    auto* interjectionContext = std::get <0>(nextParameters);
                    interjectionContext->previous = &prev;

                    if constexpr (sizeof...(List) >= 1)
                    {
                        if constexpr (std::is_same <ReturnT, bool>::value)
                        {
                            if (!interjectionContext->interjection(*interjectionContext))
                                return;
                        }
                        else
                            interjectionContext->interjection(*interjectionContext);
                        interjectionContext->count++;
                        detail::apply_non_deduce(&waterfall_interject_impl <ReturnT, std::decay_t <List> const&...>, nextParameters);
                    }
            });
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
        detail::waterfall_interject_impl(&ctx, std::forward <T>(front), std::forward <List>(list)...);
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
