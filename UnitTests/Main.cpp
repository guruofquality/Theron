// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <stdio.h>

#include "TestFramework/TestManager.h"


int main()
{
    using namespace TestFramework;

    // Run all the tests, report the result, print any errors.
    const bool allPassed = TestManager::Instance()->RunTests();
    if (!allPassed)
    {
        const TestManager::ErrorList &errors(TestManager::Instance()->GetErrors());
        printf("Tests FAILED with %d error%s\n", errors.size(), errors.size() == 1 ? "" : "s");
        return 1;
    }

    printf("PASSED\n");
    return 0;
}

