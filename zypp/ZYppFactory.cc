/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/ZYppFactory.cc
 *
*/
extern "C"
{
#include <sys/file.h>
}
#include <iostream>
#include <fstream>

#include "zypp/base/Logger.h"
#include "zypp/base/Gettext.h"
#include "zypp/base/IOStream.h"
#include "zypp/PathInfo.h"

#include "zypp/ZYppFactory.h"
#include "zypp/zypp_detail/ZYppImpl.h"
#include "zypp/zypp_detail/ZYppReadOnlyHack.h"

#define ZYPP_LOCK_FILE "/var/run/zypp.pid"

using std::endl;

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  namespace zypp_readonly_hack
  { /////////////////////////////////////////////////////////////////

    static bool active = false;

    void IWantIt()
    {
      active = true;
      MIL << "ZYPP_READONLY promised." <<  endl;
    }

    /////////////////////////////////////////////////////////////////
  } // namespace zypp_readonly_hack
  ///////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  //
  //    CLASS NAME : ZYppGlobalLock
  //
  ///////////////////////////////////////////////////////////////////

  class ZYppGlobalLock
  {
    public:

      ZYppGlobalLock()
      : _clean_lock(false)
      , _zypp_lockfile(0)
      , _locker_pid(0)
    {}

    ~ZYppGlobalLock()
    {
      try
        {
          pid_t curr_pid = getpid();
          if ( _zypp_lockfile )
            {
              Pathname lock_file = Pathname(ZYPP_LOCK_FILE);
              unLockFile();
              closeLockFile();

              if ( _clean_lock )
              {
                MIL << "Cleaning lock file. (" << curr_pid << ")" << std::endl;
                if ( filesystem::unlink(lock_file) == 0 )
                  MIL << "Lockfile cleaned. (" << curr_pid << ")" << std::endl;
                else
                  ERR << "Cant clean lockfile. (" << curr_pid << ")" << std::endl;
              }
            }
        }
      catch(...) {} // let no exception escape.
    }

    pid_t locker_pid() const
    { return _locker_pid; }

    const std::string & locker_name() const
    { return _locker_name; }


    bool _clean_lock;

    private:
    FILE *_zypp_lockfile;
    pid_t _locker_pid;
    std::string _locker_name;

    void openLockFile(const char *mode)
    {
      Pathname lock_file = Pathname(ZYPP_LOCK_FILE);
      _zypp_lockfile = fopen(lock_file.asString().c_str(), mode);
      if (_zypp_lockfile == 0)
        ZYPP_THROW (Exception( "Cant open " + lock_file.asString() + " in mode " + std::string(mode) ) );
    }

    void closeLockFile()
    {
      fclose(_zypp_lockfile);
    }

    void shLockFile()
    {
      int fd = fileno(_zypp_lockfile);
      int lock_error = flock(fd, LOCK_SH);
      if (lock_error != 0)
        ZYPP_THROW (Exception( "Cant get shared lock"));
      else
        MIL << "locked (shared)" << std::endl;
    }

    void exLockFile()
    {
      int fd = fileno(_zypp_lockfile);
    // lock access to the file
      int lock_error = flock(fd, LOCK_EX);
      if (lock_error != 0)
        ZYPP_THROW (Exception( "Cant get exclusive lock" ));
      else
        MIL << "locked (exclusive)" << std::endl;
    }

    void unLockFile()
    {
      int fd = fileno(_zypp_lockfile);
    // lock access to the file
      int lock_error = flock(fd, LOCK_UN);
      if (lock_error != 0)
        ZYPP_THROW (Exception( "Cant release lock" ));
      else
        MIL << "unlocked" << std::endl;
    }

    bool lockFileExists()
    {
      Pathname lock_file = Pathname(ZYPP_LOCK_FILE);
    // check if the file already existed.
      bool exists = PathInfo(lock_file).isExist();
      return exists;
    }

    void createLockFile()
    {
      pid_t curr_pid = getpid();
      openLockFile("w");
      exLockFile();
      fprintf(_zypp_lockfile, "%ld\n", (long) curr_pid);
      fflush(_zypp_lockfile);
      unLockFile();
      MIL << "written lockfile with pid " << curr_pid << std::endl;
      closeLockFile();
    }

    bool isProcessRunning(pid_t pid_r)
    {
      // it is another program, not me, see if it is still running
      Pathname procdir( "/proc"/str::numstring(pid_r) );

      PathInfo status( procdir/"status" );
      MIL << "Checking " <<  status << endl;
      bool still_running = status.isExist();

      if ( still_running )
      {
        Pathname p( procdir/"exe" );
        _locker_name = filesystem::readlink( p ).asString();
        MIL << p << " -> " << _locker_name << endl;

        p = procdir/"cmdline";
        MIL << p << ": ";
        std::ifstream infile( p.c_str() );
        for( iostr::EachLine in( infile ); in; in.next() )
        {
          MIL << *in << endl;
        }
      }

      return still_running;
    }

    pid_t lockerPid()
    {
      pid_t curr_pid = getpid();
      pid_t locker_pid = 0;
      long readpid = 0;

      fscanf(_zypp_lockfile, "%ld", &readpid);
      MIL << "read: Lockfile " << ZYPP_LOCK_FILE << " has pid " << readpid << " (our pid: " << curr_pid << ") "<< std::endl;
      locker_pid = (pid_t) readpid;
      return locker_pid;
    }

  public:

    bool zyppLocked()
    {
      pid_t curr_pid = getpid();
      Pathname lock_file = Pathname(ZYPP_LOCK_FILE);

      if ( lockFileExists() )
      {
        MIL << "found lockfile " << lock_file << std::endl;
        openLockFile("r");
        shLockFile();

        pid_t locker_pid = lockerPid();
        _locker_pid = locker_pid;
	if ( locker_pid == curr_pid )
        {
        // alles ok, we are requesting the instance again
          //MIL << "Lockfile found, but it is myself. Assuming same process getting zypp instance again." << std::endl;
          return false;
        }
        else
        {
          if ( isProcessRunning(locker_pid) )
          {
            if ( geteuid() == 0 )
            {
              // i am root
              MIL << locker_pid << " is running and has a ZYpp lock. Sorry" << std::endl;
              return true;
            }
            else
            {
              MIL << locker_pid << " is running and has a ZYpp lock. Access as normal user allowed." << std::endl;
              return false;
            }
          }
          else
          {
            if ( geteuid() == 0 )
            {
              MIL << locker_pid << " has a ZYpp lock, but process is not running. Cleaning lock file." << std::endl;
              if ( filesystem::unlink(lock_file) == 0 )
              {
                createLockFile();
              // now open it for reading
                openLockFile("r");
                shLockFile();
                return false;
              }
              else
              {
                ERR << "Can't clean lockfile. Sorry, can't create a new lock. Zypp still locked." << std::endl;
                return true;
              }
            }
            else
            {
              MIL << locker_pid << " is running and has a ZYpp lock. Access as normal user allowed." << std::endl;
              return false;
            }
          }
        }
      }
      else
      {
        MIL << "no lockfile " << lock_file << " found" << std::endl;
        if ( geteuid() == 0 )
        {
          MIL << "running as root. Will attempt to create " << lock_file << std::endl;
          createLockFile();
        // now open it for reading
          openLockFile("r");
          shLockFile();
        }
        else
        {
          MIL << "running as user. Skipping creating " << lock_file << std::endl;
        }
        return false;
      }
      return true;
    }

  };

  static ZYppGlobalLock globalLock;

  ///////////////////////////////////////////////////////////////////
  //
  //	CLASS NAME : ZYppFactoryException
  //
  ///////////////////////////////////////////////////////////////////

  ZYppFactoryException::ZYppFactoryException( const std::string & msg_r, pid_t locker_pid )
    : Exception(msg_r),
      _locker_pid (locker_pid)
  {}

  ///////////////////////////////////////////////////////////////////
  //
  //	CLASS NAME : ZYppFactory
  //
  ///////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZYppFactory::instance
  //	METHOD TYPE : ZYppFactory
  //
  ZYppFactory ZYppFactory::instance()
  {
    return ZYppFactory();
  }

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZYppFactory::ZYppFactory
  //	METHOD TYPE : Ctor
  //
  ZYppFactory::ZYppFactory()
  {

  }

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZYppFactory::~ZYppFactory
  //	METHOD TYPE : Dtor
  //
  ZYppFactory::~ZYppFactory()
  {}

  ///////////////////////////////////////////////////////////////////
  //
  ZYpp::Ptr ZYppFactory::getZYpp() const
  {
    static ZYpp::Ptr _instance;

    if ( ! _instance )
    {
      /*--------------------------------------------------*/
      if ( zypp_readonly_hack::active )
        {
          _instance = new ZYpp( ZYpp::Impl_Ptr(new ZYpp::Impl) );
          MIL << "ZYPP_READONLY active." << endl;
          return _instance;
        }
      /*--------------------------------------------------*/
      if ( globalLock.zyppLocked() )
      {
	std::string t = str::form(N_("System management is locked by the application with pid %d (%s).\n"
                                     "Close this application before trying again."),
                                  globalLock.locker_pid(),
                                  globalLock.locker_name().c_str()
                                 );
	ZYPP_THROW(ZYppFactoryException(t, globalLock.locker_pid()));
      }
      else
      {
        _instance = new ZYpp( ZYpp::Impl_Ptr(new ZYpp::Impl) );
        globalLock._clean_lock = true;
      }
    }

    return _instance;
  }

  /******************************************************************
  **
  **	FUNCTION NAME : operator<<
  **	FUNCTION TYPE : std::ostream &
  */
  std::ostream & operator<<( std::ostream & str, const ZYppFactory & obj )
  {
    return str << "ZYppFactory";
  }

  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
