/* libLDR: Portable and easy-to-use LDraw format abstraction & I/O reference library *
 * To obtain more information about LDraw, visit http://www.ldraw.org.               *
 * Distributed in terms of the GNU Lesser General Public License v3                  *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#ifndef _LIBLDR_METRICS_H_
#define _LIBLDR_METRICS_H_

#include <stack>

#include "extension.h"
#include "math.h"

namespace ldraw
{

class filter;
class model;

class LIBLDR_EXPORT metrics : public extension
{
  public:
    metrics(model *m, void *arg = 0L);
    metrics(const vector &min, const vector &max);
    virtual ~metrics() {}

    metrics& operator=(const metrics &rhs);
	
    static const std::string identifier() { return "metrics"; } 

    void update();
    void update(const filter *filter);

    bool is_null() const { return m_null; }
    const vector& min_() const { return m_min; }
    const vector& max_() const { return m_max; }

  private:
    void do_recursive(const model *m, std::stack<matrix> *modelview_matrix,
        const filter *filter = 0L, bool orthogonal = true, int depth = 0);
    void dimension_test(const vector &vec);
    void dimension_test(const matrix &transformation, const metrics &m);
	
  protected:
    bool m_null;
    vector m_min;
    vector m_max;
    bool m_started;
};

};

#endif
