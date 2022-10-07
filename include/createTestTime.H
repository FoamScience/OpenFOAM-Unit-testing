/*---------------------------------------------------------------------------*\
Description
    Create a time object for testing

Author
    Mohammed Elwardi Fadeli (elwardifadeli@gmail.com)

Notes
    In parallel runs, Paths to the specific processor dir need to be overriden
\*---------------------------------------------------------------------------*/

#ifndef ROOT_CASEPATH
    #define ROOT_CASEPATH fileName("")
#endif

#ifndef ROOT_CASENAME
    #define ROOT_CASENAME fileName("")
#endif

#ifndef TIME_NAME
    #define TIME_NAME runTime
#endif

// Create Sample Time object with no function objects
Time TIME_NAME
(
        word(Foam::Time::controlDictName),
        ROOT_CASEPATH,
        ROOT_CASENAME,
        word("system"),
        word("constant"),
        false
);
