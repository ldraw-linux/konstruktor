/* libLDR: Portable and easy-to-use LDraw format abstraction & I/O reference library *
 * To obtain more information about LDraw, visit http://www.ldraw.org.               *
 * Distributed in terms of the GNU Lesser General Public License v3                  *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#ifndef _LIBLDR_EXTENSION_H_
#define _LIBLDR_EXTENSION_H_

#include "common.h"

namespace ldraw
{

class model;

class LIBLDR_EXPORT extension
{
  public:
	extension(model *m, void *arg = 0L) : m_model(m), m_arg(arg) {}
	virtual ~extension() {}

	void set_data(void *arg) { m_arg = arg; }

	virtual void update() {}
	
  protected:
  	model *m_model;
	void *m_arg;
};

}

#endif
