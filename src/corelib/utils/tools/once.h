#pragma once
#include <QMetaObject>
#include <QObject>

#include <memory>
#include <type_traits>

namespace Once {

namespace Helper {
/* Helper classes to extract all but the last element of a list (ListLeft).
 * */
template<typename...>
struct List {
};
template<typename Head, typename... Tail>
struct List<Head, Tail...> {
    typedef Head First;
    typedef List<Tail...> Rest;
};
template<typename, typename>
struct ListAppend;
template<typename... L1, typename... L2>
struct ListAppend<List<L1...>, List<L2...>> {
    typedef List<L1..., L2...> value;
};
template<typename L, int N>
struct ListLeft {
    typedef typename ListAppend<List<typename L::First>,
                                typename ListLeft<typename L::Rest, N - 1>::value>::value value;
};
template<typename L>
struct ListLeft<L, 0> {
    typedef List<> value;
};

/* Helper class to return the maximum number of parameters in a function
 * that is compatible with the given list of arguments. -1 if the function
 * is incompatible. */
template<typename Func, typename ArgList>
struct CompatibleArgCount;

template<typename Func, typename ArgList, bool NotCompatible>
struct CompatibleArgCountHelper {
    /* If, after testing a smaller and smaller argument list, no
     * compatibility between the signal and slot parameters is found,
     * return -1: */
    enum { value = -1 };
};

template<typename Func, typename FirstArg, typename... Args>
/* Test again using CompatibleArgCount with the last argument removed
 * from ArgList: */
struct CompatibleArgCountHelper<Func, List<FirstArg, Args...>, false>
    : CompatibleArgCount<
          Func,
          typename ListLeft<List<FirstArg, Args...>, static_cast<int>(sizeof...(Args))>::value> {
};

template<typename Func, typename... Args>
struct CompatibleArgCount<Func, List<Args...>> {
    /* A dummy function whose only purpose is to return the give type: */
    template<typename T>
    static T dummy();
    /* A function whose return type is an integer. The function is only
     * defined if all arguments, Args, can be passed to the function F.
     * */
    template<typename F>
    static auto test(F func) -> decltype(func(dummy<Args>()...), int());
    /* A function accepting any arguments, returning char (a datatype
     * with another size than the function potentially defined above): */
    static char test(...);
    enum {
        /* If the test function is defined, then the number of arguments
         * match and the data types are compatible: */
        value = sizeof(test(dummy<Func>())) == sizeof(int)
            /* The total argument count are compatible: */
            ? static_cast<int>(sizeof...(Args))
            /* The test failed, but try again after removing the last
             * argument: */
            : static_cast<int>(CompatibleArgCountHelper<Func, List<Args...>, false>::value),
    };
};

/* Helper classes to extract a list of arguments and argument count from a
 * functor. */
template<typename Func>
struct FuncMeta {
    enum {
        ArgCount = -1,
        IsFunctor = true,
    };
};

template<typename Ret, typename... Args>
struct FuncMeta<Ret (*)(Args...)> {
    typedef List<Args...> ArgList;
    enum {
        ArgCount = sizeof...(Args),
        IsFunctor = false,
    };
};

template<typename Ret, typename Class, typename... Args>
struct FuncMeta<Ret (Class::*)(Args...)> {
    typedef List<Args...> ArgList;
    typedef Class Object;
    enum {
        ArgCount = sizeof...(Args),
        IsFunctor = false,
    };
};

template<typename Ret, typename Class, typename... Args>
struct FuncMeta<Ret (Class::*)(Args...) const> {
    typedef List<Args...> ArgList;
    typedef Class Object;
    enum {
        ArgCount = sizeof...(Args),
        IsFunctor = false,
    };
};

/* Helper classes to generate an integer sequence: Used for unpacking
 * tuples. */
template<int... I>
struct Sequence {
};
template<int N, int... I>
struct MakeSequnceHelper : MakeSequnceHelper<N - 1, N - 1, I...> {
};
template<int... I>
struct MakeSequnceHelper<0, I...> : Sequence<I...> {
};
/* This extra layer prevents a long recursion caused by attempting to use
 * N as a negative number (or a large number if an unsigned integer were
 * used in the template instead): */
template<int N, typename Enable = void>
struct MakeSequnce {
};
template<int N>
struct MakeSequnce<N, typename std::enable_if<N >= 0>::type> : MakeSequnceHelper<N> {
};
} // namespace Helper

template<typename Func, typename Signal, typename Enable = void>
struct AutoDisconnecter {
};

template<typename Func, typename Signal>
struct AutoDisconnecter<Func, Signal,
                        typename std::enable_if<Helper::FuncMeta<Func>::IsFunctor>::type> {
    typename std::remove_reference<Func>::type func;
    std::shared_ptr<QMetaObject::Connection> connection;
    mutable int remainingCalls = 1;

    AutoDisconnecter(Func&& func_, int fireCount_ = 1)
        : func(std::forward<Func>(func_))
        , connection(std::make_shared<QMetaObject::Connection>())
        , remainingCalls(fireCount_)
    {
    }

    template<typename... Args>
    void operator()(Args&&... args)
    {
        /* TODO: Figure out a better way of extracting n first arguments than
         * using a tuple and an integer sequence: */
        std::tuple<Args...> tuple(std::forward<Args>(args)...);
        /* Here is the magic. All this code just to call disconnect before
         * calling the user's functor: */
        if (--remainingCalls <= 0)
            QObject::disconnect(*connection);
        /* In case connection is queued, in which case this function might be
         * scheduled to run before the disconnect call has any effect, make sure
         * to avoid calling the functor: */
        if (remainingCalls < 0)
            return;

        /* TODO: Are more compile-time tests necessary here or are they still
         * performed in QObject::connect? */
        const int maxArgs
            = Helper::CompatibleArgCount<Func, typename Helper::FuncMeta<Signal>::ArgList>::value;
        static_assert(maxArgs != -1, "Signal and slot arguments are not compatible.");
        call(tuple, Helper::MakeSequnce<maxArgs>{});
    }

    template<typename... Args, int... I>
    void call(std::tuple<Args...>& tuple, Helper::Sequence<I...>)
    {
        func(std::get<I>(tuple)...);
    }
};

template<typename Func>
struct AutoDisconnecter<Func, typename std::enable_if<!Helper::FuncMeta<Func>::IsFunctor>::type> {
    typename std::remove_reference<Func>::type func;
    std::shared_ptr<QMetaObject::Connection> connection;
    mutable int remainingCalls = 1;

    AutoDisconnecter(Func&& func_, int fireCount_ = 1)
        : func(std::forward<Func>(func_))
        , connection(std::make_shared<QMetaObject::Connection>())
        , remainingCalls(fireCount_)
    {
    }

    template<typename... Args, int... I>
    void call(std::tuple<Args...>& tuple, Helper::Sequence<I...>)
    {
        func(std::get<I>(tuple)...);
    }

    template<typename... Args>
    void operator()(Args&&... args)
    {
        /* TODO: Figure out a better way of extracting n first arguments than
         * using a tuple and an integer sequence: */
        std::tuple<Args...> a(std::forward<Args>(args)...);
        /* Here is the magic. All this code just to call disconnect before
         * calling the user's functor: */
        if (--remainingCalls <= 0)
            QObject::disconnect(*connection);
        /* In case connection is queued, in which case this function might be
         * scheduled to run before the disconnect call has any effect, make sure
         * to avoid calling the functor: */
        if (remainingCalls < 0)
            return;

        call(a, Helper::MakeSequnce<Helper::FuncMeta<Func>::ArgCount>{});
    }
};

template<typename Receiver, typename Slot, typename Signal>
struct SlotAutoDisconnecter {
    Receiver* receiver;
    Slot slot;
    std::shared_ptr<QMetaObject::Connection> connection;
    mutable int remainingCalls = 1;

    SlotAutoDisconnecter(Receiver* receiver_, Slot&& slot_, int fireCount_ = 1)
        : receiver(receiver_)
        , slot(std::forward<Slot>(slot_))
        , connection(std::make_shared<QMetaObject::Connection>())
        , remainingCalls(fireCount_)
    {
    }

    template<typename... Args, int... I>
    void call(std::tuple<Args...>& tuple, Helper::Sequence<I...>)
    {
        (receiver->*slot)(std::get<I>(tuple)...);
    }

    template<typename... Args>
    void operator()(Args&&... args)
    {
        /* TODO: Figure out a better way of extracting n first arguments than
         * using a tuple and an integer sequence: */
        std::tuple<Args...> a(std::forward<Args>(args)...);
        /* Here is the magic. All this code just to call disconnect before
         * calling the user's functor: */
        if (--remainingCalls <= 0)
            QObject::disconnect(*connection);
        /* In case connection is queued, in which case this function might be
         * scheduled to run before the disconnect call has any effect, make sure
         * to avoid calling the functor: */
        if (remainingCalls < 0)
            return;

        call(a, Helper::MakeSequnce<Helper::FuncMeta<Slot>::ArgCount>{});
    }
};

template<typename Object, typename Signal, typename Functor>
QMetaObject::Connection connect(const Object* sender, Signal signal, Functor&& functor)
{
    AutoDisconnecter<Functor, Signal> disconnecter(std::forward<Functor>(functor));
    return *disconnecter.connection = QObject::connect(sender, signal, disconnecter);
}

template<typename Object, typename Signal, typename Functor>
QMetaObject::Connection connect(const Object* sender, Signal signal, const QObject* context,
                                Functor&& functor, Qt::ConnectionType type = Qt::AutoConnection)
{
    AutoDisconnecter<Functor, Signal> disconnecter(std::forward<Functor>(functor));
    return *disconnecter.connection = QObject::connect(sender, signal, context, disconnecter, type);
}

template<typename Object, typename Signal, typename Functor>
QMetaObject::Connection connect(const Object* sender, Signal signal, int callCount,
                                Functor&& functor)
{
    /* For convenice, allow user to call a function 0 times: */
    if (callCount <= 0)
        return {};

    AutoDisconnecter<Functor, Signal> disconnecter(std::forward<Functor>(functor), callCount);
    return *disconnecter.connection = QObject::connect(sender, signal, disconnecter);
}

template<typename Object, typename Signal, typename Functor>
QMetaObject::Connection connect(const Object* sender, Signal signal, int callCount,
                                const QObject* context, Functor&& functor,
                                Qt::ConnectionType type = Qt::AutoConnection)
{
    /* For convenice, allow user to call a function 0 times: */
    if (callCount <= 0)
        return {};

    AutoDisconnecter<Functor, Signal> disconnecter(std::forward<Functor>(functor), callCount);
    return *disconnecter.connection = QObject::connect(sender, signal, context, disconnecter, type);
}

template<typename Sender, typename Signal, typename Slot>
QMetaObject::Connection connect(const Sender* sender, Signal signal,
                                const typename Helper::FuncMeta<Slot>::Object* receiver, Slot slot,
                                Qt::ConnectionType type = Qt::AutoConnection)
{
    using Receiver = typename Helper::FuncMeta<Slot>::Object;
    SlotAutoDisconnecter<Receiver, Slot, Signal> disconnecter(const_cast<Receiver*>(receiver),
                                                              std::forward<Slot>(slot));
    /* Use receiver as context: */
    return *disconnecter.connection
        = QObject::connect(sender, signal, receiver, disconnecter, type);
}

template<typename Sender, typename Signal, typename Slot>
QMetaObject::Connection connect(const Sender* sender, Signal signal, int callCount,
                                const typename Helper::FuncMeta<Slot>::Object* receiver, Slot slot,
                                Qt::ConnectionType type = Qt::AutoConnection)
{
    /* For convenice, allow user to call a function 0 times: */
    if (callCount <= 0)
        return {};

    using Receiver = typename Helper::FuncMeta<Slot>::Object;
    SlotAutoDisconnecter<Receiver, Slot, Signal> disconnecter(const_cast<Receiver*>(receiver),
                                                              std::forward<Slot>(slot), callCount);
    /* Use receiver as context: */
    return *disconnecter.connection
        = QObject::connect(sender, signal, receiver, disconnecter, type);
}

} // namespace Once
