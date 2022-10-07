/*---------------------------------------------------------------------------*\
Description
    A set of macro to ease writing unit tests

Author
    Mohammed Elwardi Fadeli (elwardifadeli@gmail.com)

Notes
    Macros for creation/manipulation of time/mesh/field objects
\*---------------------------------------------------------------------------*/

#ifndef testMacros_H
#define testMacros_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include <mpi.h>
#include "Pstream.H"
#include "memberStealer.H"

// 1 Time-related macros

// Specialize the member stealer for case and processorCase
#define prepareTimePaths()                                          \
   SPECIALIZE_MEMBER_STEALER(globalCaseName_, fileName, TimePaths);           \
   SPECIALIZE_MEMBER_STEALER(processorCase_, bool, TimePaths)

// Reset time case name and processorCase
#define modifyTimePaths(timeName, newProcessorCase, newCaseName)    \
    GET_REF(processorCase_, TimePaths, timeName) = newProcessorCase;\
    GET_REF(globalCaseName_, TimePaths, timeName) = newCaseName

// 2 Mesh-related macros

// Reset mesh pointer Sample dynamic Mesh object
#define resetMeshPointer(timeObj, meshPtr, meshClass, region)      \
meshPtr.reset                                                      \
(                                                                  \
    new meshClass                                                  \
    (                                                              \
        IOobject                                                   \
        (                                                          \
            region,                                                \
            timeObj.timeName(),                                    \
            timeObj,                                               \
            Foam::IOobject::MUST_READ                              \
        )                                                          \
    )                                                              \
)

// 3 Field-related macros

// Create a field from an IOobject, mesh, and initial value
#define createField(fieldName, fieldClass, elementClass, time, mesh, dimSet, zero, defaultBC) \
fieldClass fieldName                                               \
(                                                                  \
    IOobject                                                       \
    (                                                              \
        #fieldName,                                                \
        time.timeName(),                                           \
        mesh,                                                      \
        IOobject::NO_READ,                                         \
        IOobject::AUTO_WRITE                                       \
    ),                                                             \
    mesh,                                                          \
    elementClass(#fieldName, dimSet, zero),                        \
    #defaultBC                                                     \
)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#endif

// ************************************************************************* //
