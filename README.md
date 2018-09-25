# promise-waterfall
This header only library adds a thing comparable to the promise concept.
Providing a tool to do asynchronous work in sequence by chaining callbacks.
Especially useful for I/O where 

A useful tool called waterfall allows to write sequentially looking, but actually asynchronous code.

#Examples
##Example 0
```C++
#define PROMISE_WATERFALL_NO_BOOST
#include "promise.hpp"

#include <iostream>

int main()
{
    using namespace PromiseWaterfall;

	Promise p;
	p.then([](){
		std::cout << "bla\n";
	}).except([](auto ec){
        std::cout << "oh no!\n";
        std::cout << ec << "\n";
    });

    // -> "bla"
    //p.fullfill();

    // -> "oh no!"
    p.error(50);
}
```

##Example 1
```C++
#include "waterfall.hpp"

#include <iostream>

using namespace PromiseWaterfall;

Promise _1;

Promise& foo()
{
    std::cout << "foo\n";
    return _1;
}
Promise bar()
{
    std::cout << "bar\n";
    return Promise{}.fullfill();
}

int main()
{
    waterfall
    (
        foo,
        bar,
        []() -> Promise {std::cout << "lambda\n"; return Promise{}.fullfill(); }
    );
    std::cout << "after waterfall\n";
    _1.fullfill();
    std::cout << "after all\n";

    std::cin.get();
    return 0;
}
```
Will produce
```
foo
after waterfall
bar
lambda
after all
```



##Example 2
```C++
#include "waterfall.hpp"

#include <iostream>

using namespace PromiseWaterfall;

Promise _1;

Promise& foo()
{
    std::cout << "foo\n";
    return _1;
}
Promise bar()
{
    std::cout << "bar\n";
    return Promise{}.fullfill();
}

int main()
{
    waterfall_interject <void>
    (
		// first function is called in between each of the promises.
		// useful for adding pauses or repeated intermediary work.
        [](auto& ctx) {std::cout << "---> " << ctx.count << " <---\n"; },
        foo,
        bar,
        []() -> Promise {std::cout << "lambda\n"; return Promise{}.fullfill(); }
    );
    _1.fullfill();
    return 0;
}

```
Will produce
```
foo
---> 0 <---
bar
---> 1 <---
lambda
```

##Example 3
```C++
#include "waterfall.hpp"

/* ... */

int main()
{
    waterfall_interject <bool>
    (
		// return true to continue, false to break chain without error.
        [](auto& ctx) {std::cout << "---> " << ctx.count << " <---\n"; return true;},
        foo,
        bar
    );
    return 0;
}
```
