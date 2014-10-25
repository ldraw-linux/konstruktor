/* libLDR: Portable and easy-to-use LDraw format abstraction & I/O reference library *
 * To obtain more information about LDraw, visit http://www.ldraw.org.               *
 * Distributed in terms of the GNU Lesser General Public License v3                  *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#ifndef _LIBLDR_WRITER_H_
#define _LIBLDR_WRITER_H_

#include <ostream>
#include <string>

#include "common.h"

namespace ldraw
{

class model;
class model_multipart;
class element_base;
class element_comment;
class element_state;
class element_print;
class element_ref;
class element_line;
class element_triangle;
class element_quadrilateral;
class element_condline;
class element_bfc;
class matrix;
class vector;

class LIBLDR_EXPORT writer
{
  public:
	writer(const std::string &filename);
	writer(std::ostream &stream);
	virtual ~writer();

	void write(const model *model);
	void write(const model_multipart *mpmodel);
	void write(const element_base *elem);

	void serialize_comment(const element_comment *e);
	void serialize_state(const element_state *e);
	void serialize_print(const element_print *e);
	void serialize_ref(const element_ref *e);
	void serialize_line(const element_line *e);
	void serialize_triangle(const element_triangle *e);
	void serialize_quadrilateral(const element_quadrilateral *e);
	void serialize_condline(const element_condline *e);
	void serialize_bfc(const element_bfc *e);
	void serialize_matrix(const matrix &m);
	void serialize_vector(const vector &v);
	
  private:
	std::ofstream *m_filestream;
	std::ostream &m_stream;
};

}

#endif

