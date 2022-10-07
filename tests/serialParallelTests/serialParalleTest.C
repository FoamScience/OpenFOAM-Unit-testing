/*---------------------------------------------------------------------------*\
Description
    Tests for the testing framework itself;
    to showcase how the test framework works

Author
    Mohammed Elwardi Fadeli (elwardifadeli@gmail.com)
\*---------------------------------------------------------------------------*/

#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"
#include "fvCFD.H"
#include "testMacros.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

namespace serialParallel
{
    // Requirement 0.1: Time for parallel runs (only if you need it obviously)
    #include "createTestTime.H"
    
    // Requirement 0.1: Pointer for mesh (again if mesh is needed)
    autoPtr<fvMesh> meshPtr;
}
using namespace serialParallel;

// This is not necessary for serial runs, but gives access to some private
// members of the time object
prepareTimePaths();

// This test case shows how to write one test case for both serial and parallel
// settings. As you develop your lib, you'll want to add support for parallel
// cases as you go!
// - Tag the test case with [Serial] and/or [Parallel] if it's supposed to run in
// both/either modes
// - Tag the test case with [Case_caseName] if you want it to run on the OpenFOAM
// caseName case
// - Tag the test case with [nProcs_2] to specify with how many cores you want it to run
TEST_CASE
(
    "Support for serial and parallel runs",
    "[Serial][Parallel][Case_cavity]"
)
{
    // Turn this on if you want to see FATAL ERROR msgs
    //FatalError.dontThrowExceptions();

    if (Pstream::parRun()) {
        // If parallel run, fix case paths - assuming you ran the binary from the case
        // directory
        word procPath = "processor"+Foam::name(Pstream::myProcNo());
        modifyTimePaths(runTime, true, procPath);
    }

    // Read mesh from case dir
    resetMeshPointer
    (
        runTime,
        meshPtr,
        fvMesh,
        fvMesh::defaultRegion
    );
    auto& mesh = meshPtr();

    SECTION("Decomposed mesh in parallel runs") {
        if (Pstream::parRun()) {
            // Check if the mesh was read from the correct directory
            label gNCells = returnReduce(mesh.nCells(), sumOp<label>());
            // Capture local and global mesh size, and wether we're running in parallel
            CAPTURE
            (
                mesh.nCells(),
                gNCells,
                Pstream::parRun(),
                runTime.caseName()
            );
            // This check is ran only once
            REQUIRE(mesh.nCells() < gNCells);
        }
    }

    // Generate some parameter values (1, 2 and 3)
    auto param = GENERATE( range(1,4) );

    // Capture generated value
    CAPTURE
    (
        param
    );
    
    SECTION("Volume field creation") {
        // Create a test field, populated by the param value
        createField(vf, volScalarField, dimensionedScalar, runTime, mesh, dimVolume, param, calculated);

        // Essentially, test the volScalarField constructor which takes in a init value
        REQUIRE(gMax(vf) == param);
        REQUIRE(gMin(vf) == param);
    }

    // Do not forget to clear the mesh every time!
    meshPtr.clear();
}

#include "undefineTestMacros.H"

// ************************************************************************* //
