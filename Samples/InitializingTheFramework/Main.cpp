// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample shows how to initialize a framework.
//


#include <stdio.h>

#include <Theron/Framework.h>


int main()
{
    {
        // Every Theron-based program will need to create at least one Framework
        // object. The Framework is the central manager class that hosts and executes
        // the actors in the program.
        
        // In this example the framework is created on the stack, which is the
        // easiest way to do it. The framework class itself is quite lightweight
        // and mainly uses dynamically-allocated memory for its internal storage.
        // This memory is allocated with an allocator supplied by the
        // AllocatorManager.
        printf("Constructing a framework\n");
        Theron::Framework framework;

        // Because this example is just showing how to initialize a framework,
        // we don't bother to actually use the framework to create or run any actors.
        // See the other samples for examples of that.

        // In this example, because the framework was created on the stack, it will
        // be destructed when it goes out of scope. When the framework object is
        // destructed, it automatically shuts down and frees all of its internally
        // allocated memory.
        printf("Destructing the framework\n");
    }

    {
        // Previously we used the default constructor of the Framework class to
        // construct our framework. There is also an explicit constructor that
        // allows us to provide the number of worker threads as a parameter.
        // The default value, used by the default constructor, is two.
        // Here we explicitly construct a framework with three threads instead.
        // In general, frameworks can be created with any positive number of threads.
        printf("Constructing a framework with 3 worker threads\n");
        Theron::Framework framework(3);

        // The framework will be destructed when it goes out of scope.
        printf("Destructing the framework\n");
    }

    return 0;
}

