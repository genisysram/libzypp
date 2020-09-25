#include "private/threaddata_p.h"
#include "private/eventdispatcher_glib_p.h"
#include <zypp/base/Logger.h>
#include <ostream> //for std::endl
#include <sstream>
#include <pthread.h>

namespace zyppng
{
  ThreadData::ThreadData()
  : _threadId( std::this_thread::get_id() ),
    _nativeHandle( pthread_self() )
  {
  }

  ThreadData &ThreadData::current()
  {
    static thread_local ThreadData data;
    return data;
  }

  const std::string &ThreadData::name() const
  {
    if ( _threadName.empty() ) {
      std::stringstream strStr;
      strStr << _threadId;
      _threadName = strStr.str();
    }
    return _threadName;
  }

  std::shared_ptr<EventDispatcher> ThreadData::ensureDispatcher()
  {
    auto sp = _dispatcher.lock();
    if (!sp) {
      _dispatcher = sp = EventDispatcherPrivate::create();
    }
    return sp;
  }

  void ThreadData::setDispatcher( const std::shared_ptr<EventDispatcher> &disp )
  {
    if ( _dispatcher.lock() ) {
      WAR << "Dispatcher was already created for the current thread" << std::endl;
      return;
    }
    _dispatcher = disp;
  }

  void ThreadData::syncNativeName()
  {
    pthread_setname_np( _nativeHandle, _threadName.c_str() );
  }
}
