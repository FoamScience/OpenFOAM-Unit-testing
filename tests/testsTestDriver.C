#include "catch2/catch_all.hpp"
#include "error.H"
#include <mpi.h>
#include "Pstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    // Sane OpenFOAM settings; optimized for unit testing
    // Ignore warnings
    Foam::Warning.level = 0;
    // Typically you want exceptions so the other tests continue to run;
    // because a FATAL ERROR will exit/abort
    // But **while write the test itself** you'll want to turn this off so you
    // can see the errors you're getting
    Foam::FatalError.throwExceptions();

    // Grab a catch session
    Catch::Session session;

    // Hook to arguments parser to allow parallel option
    //bool parallel = false;
    bool inParallel = false;
    auto cli = session.cli()
        | Catch::Clara::Opt(inParallel)
          ["-p"]["--parallel"]
          ("For parallel runs, Ignored by catch - used by Pstream");

    // Update CLI args and return on error
    session.cli(cli);
    auto result = session.applyCommandLine(argc, argv);
    if (result != 0) return result;

    // Init MPI comms if requested
    if (inParallel)
    {
        // OK, init MPI comms if a parallel run,
        // but this is not enough, need to point time to the correct processor path
        Foam::Pstream::init(argc,argv, 0);
    }

    // Run tests and return error code
    auto err = session.run();

    // Finialize MPI comms;
    if (inParallel)
    {
        //Foam::Pstream::waitRequests();
        MPI_Finalize();
    }

    return err;
}

// ************************************************************************* //
