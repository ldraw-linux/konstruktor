/* libLDR: Portable and easy-to-use LDraw format abstraction & I/O reference library *
 * To obtain more information about LDraw, visit http://www.ldraw.org.               *
 * Distributed in terms of the GNU Lesser General Public License v3                  *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#ifndef _LIBLDR_EXCEPTION_H_
#define _LIBLDR_EXCEPTION_H_

#include <exception>
#include <string>

#ifndef __func__
#define __func__ __FUNCTION__
#endif

namespace ldraw {

class LIBLDR_EXPORT exception : public std::exception
{
	public:
		enum error_type { warning, user_error, fatal, fixme };
		
		explicit exception(const std::string &location, error_type type, const std::string &details)
			: m_location(location), m_type(type), m_details(details) {}
		virtual ~exception() throw() {}
		
		virtual const char* what() const throw() {
			std::string s;
			
			switch(m_type) {
				case warning:
					s = "warning";
					break;
				case user_error:
					s = "user_error";
					break;
				case fatal:
					s = "error";
					break;
				case fixme:
					s  = "fixme";
					break;
				default:
					s = "";
			}
			
			return (std::string("[libLDR] ") + s + " in " + m_location + "(): " + m_details).c_str();
		}
		
		const std::string location() const { return m_location; }
		error_type type() const { return m_type; }
		const std::string details() const { return m_details; }
		
	protected:
		std::string m_location;
		error_type m_type;
		std::string m_details;
};

}

#undef EXCEPTION_EXPORT

#endif
