#ifndef INCLUDE_FLEXIBITY_SYSLOG_H
#define INCLUDE_FLEXIBITY_SYSLOG_H

#include <iostream>
#include <streambuf>
#include <string>
#include <syslog.h>
#include <stdarg.h>
#include <stdexcept>
#include <sstream>

#define SYSLOGPP_DEFAULT_PRIO LOG_ALERT

namespace Flexibity {
	class syslogbuf: public std::stringbuf {
	protected:
		int     m_prio;

		syslogbuf(int prio) : m_prio(prio) {}
		void setprio(int prio) {
			pubsync();
			m_prio = prio;
		}

		int sync() {
			if ( in_avail() )
			{
				sputc( '\0' );
				::syslog( m_prio, "%s", gptr() );
				seekoff( 0, std::ios_base::beg, std::ios_base::in | std::ios_base::out );
				setg( pbase(), pbase(), pbase() );
			}
			return 0; //success
		}
	};

	class syslog_t:	protected syslogbuf, public std::ostream {
	public:
		syslog_t(int prio = SYSLOGPP_DEFAULT_PRIO) : syslogbuf(prio), std::ostream( static_cast<std::streambuf*>(this) ) {}

		~syslog_t() {
			pubsync();
		}

		//operator of priority modification
		std::ostream&   operator () (int prio) {
			setprio( prio );
			return *this;
		}

		//standard interface
		void open(const char* procname, int option, int facility) {
			::openlog( procname, option, facility );
		}

		void close() {
			::closelog();
		}

		int setmask(int mask) {
			return setlogmask( mask );
		}

		void operator () (int prio, const char* fmt, ...) const {
			va_list args;
			va_start( args, fmt );
			::vsyslog( prio, fmt, args );
			va_end( args );
		}
	};

	syslog_t& syslog_get() throw(std::bad_alloc,std::runtime_error) {
		static syslog_t g_syslog;
		return g_syslog;
	}
}

#endif // INCLUDE_FLEXIBITY_SYSLOG_H
