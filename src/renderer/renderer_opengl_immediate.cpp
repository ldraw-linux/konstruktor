/* LDRrenderer: LDraw model rendering library which based on libLDR                  *
 * To obtain more information about LDraw, visit http://www.ldraw.org                *
 * Distributed in terms of the General Public License v2                             *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#include <cmath>
#include <string>

#include <libldr/color.h>
#include <libldr/elements.h>
#include <libldr/filter.h>
#include <libldr/math.h>
#include <libldr/metrics.h>
#include <libldr/utils.h>

#include "opengl.h"
#include "normal_extension.h"
#include "parameters.h"

#include "renderer_opengl_immediate.h"

namespace ldraw_renderer
{

renderer_opengl_immediate::renderer_opengl_immediate(const parameters *rp)
	: renderer_opengl(rp)
{
}

renderer_opengl_immediate::~renderer_opengl_immediate()
{
}

void renderer_opengl_immediate::render(ldraw::model *m, const ldraw::filter *filter)
{
	std::memset(&m_stats, 0, sizeof(statistics));
	
	switch (m_params->get_rendering_mode()) {
		case parameters::model_full:
			draw_model_full(m->parent(), m, 0, filter);
			return;
		case parameters::model_edges:
			draw_model_edges(m->parent(), m, 0, filter);
			return;
		case parameters::model_boundingboxes:
			glPointSize(5.0f);
			draw_model_bounding_boxes(m->parent(), m, 0, filter);
	}
}

// rendering code
void renderer_opengl_immediate::draw_model_full(const ldraw::model_multipart *base, ldraw::model *m, int depth, const ldraw::filter *filter)
{
	bool culling = true;
	bool invertnext = false;
	ldraw::bfc_certification::winding winding = ldraw::bfc_certification::ccw;
	ldraw::bfc_certification::winding localwinding = ldraw::bfc_certification::ccw;
	ldraw::bfc_certification::cert_status cert;
	const ldraw::bfc_certification *cext = m->custom_data<ldraw::bfc_certification>();
	bool cullenabled = false;

	if (!cext)
		cert = ldraw::bfc_certification::unknown;
	else {
		cert = cext->certification();
		if (cert == ldraw::bfc_certification::certified) {
			winding = localwinding = cext->orientation();
			if (m_bfc_tracker.inverted())
				winding = winding == ldraw::bfc_certification::ccw ? ldraw::bfc_certification::cw : ldraw::bfc_certification::ccw;
			if (m_bfc_tracker.localinverted())
				localwinding = localwinding == ldraw::bfc_certification::ccw ? ldraw::bfc_certification::cw : ldraw::bfc_certification::ccw;
		}
	}

	// Obtain current modelview matrix for conditional line calculation
	ldraw::matrix proj;
	glGetFloatv(GL_MODELVIEW_MATRIX, const_cast<float *>(proj.get_pointer()));
	proj = proj.transpose();
	
	// enable shading if set
	normal_extension *ne = 0L;
	if (!m->custom_data<normal_extension>())
		m->update_custom_data<normal_extension>();

	ne = m->custom_data<normal_extension>();

	/* apply bfc policy */
	if (cert == ldraw::bfc_certification::certified && culling && m_bfc_tracker.culling() && m_params->get_culling()) {
		glEnable(GL_CULL_FACE);
		cullenabled = true;
	} else {
		glDisable(GL_CULL_FACE);
		cullenabled = false;
	}

	bool flipped = ldraw::utils::det3(proj) < 0.0f;

	// Iterate!
	int i = 0;
	for (ldraw::model::const_iterator it = m->elements().begin(); it != m->elements().end(); ++it) {
		if (filter && filter->query(m, i, depth)) {
			++i;
			continue;
		}
		
		ldraw::type elemtype = (*it)->get_type();

		if (elemtype == ldraw::type_triangle || elemtype == ldraw::type_quadrilateral) {
			GLenum mode;
			
			if (winding == ldraw::bfc_certification::ccw) {
				if (flipped)
					mode = GL_CW;
				else
					mode = GL_CCW;
			} else {
				if (flipped)
					mode = GL_CCW;
				else
					mode = GL_CW;
			}

			if (m_params->get_culling())
				glFrontFace(mode);
			
			/* shading */
			if (m_params->get_shading())
				glEnable(GL_LIGHTING);
			
			if (ne->has_normal(i)) {
				ldraw::vector nv = ne->normal(i);
				
				if (winding == ldraw::bfc_certification::cw)
					nv = -nv;
				
				if (m_params->get_debug())
					render_normal_orientation(*it, nv, localwinding == ldraw::bfc_certification::ccw);
				
				if (m_params->get_shading())
					glNormal3fv(nv.get_pointer());
			}
		} else if ((elemtype == ldraw::type_line || elemtype == ldraw::type_condline) && m_params->get_shading()) {
			glDisable(GL_LIGHTING);
		}

		if (elemtype == ldraw::type_line) {
			// Line
			ldraw::element_line *l = CAST_AS_LINE(*it);
			render_line(*l);
		} else if (elemtype == ldraw::type_triangle) {
			// Triangle
			ldraw::element_triangle *l = CAST_AS_TRIANGLE(*it);
			render_triangle(*l);
		} else if (elemtype == ldraw::type_quadrilateral) {
			// Quadrilateral
			ldraw::element_quadrilateral *l = CAST_AS_QUADRILATERAL(*it);
			render_quadrilateral(*l);
		} else if (elemtype == ldraw::type_condline) {
			// Conditional line
			ldraw::element_condline *l = CAST_AS_CONDLINE(*it);
			render_condline(*l, proj);
		} else if (elemtype == ldraw::type_ref) {
			// Reference
			ldraw::element_ref *l = CAST_AS_REF(*it);
			ldraw::model *lm = l->get_model();

			if (lm) {
				// flip plane check
				bool reverse;

				if (ldraw::utils::det3(proj * l->get_matrix()) < 0.0f) {
					reverse = true;
				} else
					reverse = false;

				// Push appropriate color into the color stack.
				int id = l->get_color().get_id();
				if (id == 16 || id == 24)
					m_colorstack.push(m_colorstack.top());
				else
					m_colorstack.push(l->get_color());
	
				m_bfc_tracker.accumulate_culling(culling);
				m_bfc_tracker.accumulate_invert(invertnext, reverse);

				glPushAttrib(GL_ENABLE_BIT);
	
				// transform
				glPushMatrix();
				glMultMatrixf(l->get_matrix().transpose().get_pointer());
			
				if (ldraw::utils::is_stud(l))
					render_stud(lm, false);
				else
					draw_model_full(base, lm, depth+1, filter); // Recurse
					
				glPopMatrix();

				glPopAttrib();
	
				m_bfc_tracker.pop_culling();
				m_bfc_tracker.pop_invert();
				
				m_colorstack.pop();
			}
		} else if (elemtype == ldraw::type_bfc) {
			// Back Face Culling
			const ldraw::element_bfc *l = CAST_AS_CONST_BFC(*it);

			if (l->get_command() & ldraw::element_bfc::clip)
				culling = true;
			else if (l->get_command() == ldraw::element_bfc::noclip)
				culling = false;

			if (l->get_command() & ldraw::element_bfc::cw)
				winding = localwinding = ldraw::bfc_certification::cw;
			else if (l->get_command() & ldraw::element_bfc::ccw)
				winding = localwinding = ldraw::bfc_certification::ccw;
			
			if (m_bfc_tracker.inverted())
				winding = winding == ldraw::bfc_certification::cw ? ldraw::bfc_certification::ccw : ldraw::bfc_certification::cw;
			if (m_bfc_tracker.localinverted())
				localwinding = localwinding == ldraw::bfc_certification::cw ? ldraw::bfc_certification::ccw : ldraw::bfc_certification::cw;
			
			if (l->get_command() == ldraw::element_bfc::invertnext)
				invertnext = true;
		}

		/* reset invertnext */
		if (invertnext) {
			if (elemtype != ldraw::type_bfc) {
				invertnext = false;
			} else {
				const ldraw::element_bfc *l = CAST_AS_CONST_BFC(*it);
				
				if (l->get_command() != ldraw::element_bfc::invertnext)
					invertnext = false;
			}
		}
		
		++i;
	}
}

void renderer_opengl_immediate::draw_model_edges(const ldraw::model_multipart *base, const ldraw::model *m, int depth, const ldraw::filter *filter)
{
	// Obtain current ldraw::modelview ldraw::matrix for conditional line calculation
	float *tmp = new float[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, tmp);
	ldraw::matrix proj = ldraw::matrix(tmp).transpose();
	delete [] tmp;
	
	// Iterate!
	int i = 0;
	for (ldraw::model::const_iterator it = m->elements().begin(); it != m->elements().end(); ++it) {
		if (filter && filter->query(m, i, depth)) {
			++i;
			continue;
		}
		
		ldraw::type elemtype = (*it)->get_type();
		
		if (elemtype == ldraw::type_line) {
			// Line
			ldraw::element_line *l = CAST_AS_LINE(*it);
			render_line(*l);
		} else if (elemtype == ldraw::type_condline) {
			// Conditional line
			ldraw::element_condline *l = CAST_AS_CONDLINE(*it);
			render_condline(*l, proj);
		} else if (elemtype == ldraw::type_ref) {
			// Reference
			ldraw::element_ref *l = CAST_AS_REF(*it);
			
			// Push appropriate color into the color stack.
			int id = l->get_color().get_id();
			if (id == 16 || id == 24)
				m_colorstack.push(m_colorstack.top());
			else
				m_colorstack.push(l->get_color());
				
			// transform
			glPushMatrix();
			glMultMatrixf(l->get_matrix().transpose().get_pointer());
			
			if (ldraw::utils::is_stud(l))
				render_stud(l->get_model(), true);
			else if (l->get_model())
				draw_model_edges(base, l->get_model(), depth+1, filter); // Recurse
				
			glPopMatrix();
			m_colorstack.pop();
		}
		
		++i;
	}
}

/* Renders bounding box recursively */
void renderer_opengl_immediate::draw_model_bounding_boxes(const ldraw::model_multipart *, const ldraw::model *m, int, const ldraw::filter *filter)
{
	// Iterate!
	int i = 0;
	for (ldraw::model::const_iterator it = m->elements().begin(); it != m->elements().end(); ++it) {
		if (filter && filter->query(m, i, 0)) {
			++i;
			continue;
		}
		
		ldraw::type elemtype = (*it)->get_type();
			
		if (elemtype == ldraw::type_ref) {
			ldraw::element_ref *l = CAST_AS_REF(*it);
			
			if (l->get_model() && !l->get_model()->custom_data<ldraw::metrics>())
				l->get_model()->update_custom_data<ldraw::metrics>();
			
			glPushMatrix();
			glMultMatrixf(l->get_matrix().transpose().get_pointer());
			
			if (l->get_model()) {
				const ldraw::metrics *metrics = l->get_model()->custom_data<ldraw::metrics>();
				ldraw::vector center = (metrics->min_() + metrics->max_()) * 0.5f;
				glColor4ub(0, 0, 0, 160);
				glBegin(GL_POINTS);
				glVertex3fv(center.get_pointer());
				glEnd();
				glColor3ub(0, 0, 0);
				render_bounding_box(*metrics);
				
				//draw_model_bounding_boxes(base, l->get_model(), depth+1, std::set<int>());
			}
			
			glPopMatrix();
		}
		
		++i;
	}
}

bool renderer_opengl_immediate::hit_test(float *projection_matrix, float *modelview_matrix, int x, int y, int w, int h, ldraw::model *m, const ldraw::filter *skip_filter)
{
	GLint viewport[4];
	GLuint selectionBuffer[4];
	
	if (w == 0)
		w = 1;
	else if (w < 0)
		x += w, w = -w;

	if (h == 0)
		h = 1;
	else if (h < 0)
		y += h, h = -h;

	glSelectBuffer(4, selectionBuffer);
	glRenderMode(GL_SELECT);
	
	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix(x + w/2, viewport[3] - (y + h/2), w, h, viewport);
	glMultMatrixf(projection_matrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview_matrix);
	glInitNames();
	glPushName(0);

	glPointSize(7.0f);

	int i = 0;
	for (ldraw::model::const_iterator it = m->elements().begin(); it != m->elements().end(); ++it) {
		ldraw::type elemtype = (*it)->get_type();

		if (skip_filter->query(m, i, 0)) {
			if (elemtype == ldraw::type_ref) {
				ldraw::element_ref *l = CAST_AS_REF(*it);
				
				if (!l->get_model())
					continue;
				
				if (!l->get_model()->custom_data<ldraw::metrics>())
					l->get_model()->update_custom_data<ldraw::metrics>();
				
				glPushMatrix();
				glMultMatrixf(l->get_matrix().transpose().get_pointer());
				
				render_filled_bounding_box(*l->get_model()->custom_data<ldraw::metrics>());
				
				glPopMatrix();
			}
		}

		++i;
	}

	if (i == 0)
		return false;

	if (glRenderMode(GL_RENDER))
		return true;
	else
		return false;
}

// Get selection
selection_list renderer_opengl_immediate::select(float *projection_matrix, float *modelview_matrix, int x, int y, int w, int h, ldraw::model *m, const ldraw::filter *skip_filter)
{
	GLint hits, viewport[4];
	GLuint selectionBuffer[1024];

	if (w == 0)
		w = 1;
	else if (w < 0)
		x += w, w = -w;

	if (h == 0)
		h = 1;
	else if (h < 0)
		y += h, h = -h;

	glSelectBuffer(1024, selectionBuffer);
	glRenderMode(GL_SELECT);
	
	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix(x + w/2, viewport[3] - (y + h/2), w, h, viewport);
	glMultMatrixf(projection_matrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview_matrix);
	glInitNames();
	glPushName(0);

	glPointSize(7.0f);

	// Iterate!
	int i = 0;
	for (ldraw::model::const_iterator it = m->elements().begin(); it != m->elements().end(); ++it) {
 		if (skip_filter->query(m, i, 0)) {
			++i;
			continue;
		}
		
		ldraw::type elemtype = (*it)->get_type();
			
		if (elemtype == ldraw::type_ref) {
			ldraw::element_ref *l = CAST_AS_REF(*it);

			if (!l->get_model()) {
				++i;
				continue;
			}
			
			if (!l->get_model()->custom_data<ldraw::metrics>())
				l->get_model()->update_custom_data<ldraw::metrics>();
			
			glPushMatrix();
			glMultMatrixf(l->get_matrix().transpose().get_pointer());
			
			glLoadName(i);
			const ldraw::metrics *metrics = l->get_model()->custom_data<ldraw::metrics>();
			ldraw::vector center = (metrics->min_() + metrics->max_()) * 0.5;
			
			glBegin(GL_POINTS);
			glVertex3fv(center.get_pointer());
			glEnd();
			
			glPopMatrix();
		}
		
		++i;
	}

	hits = glRenderMode(GL_RENDER);

	std::list<std::pair<int, GLuint> > result;
	for (int i = 0; i < hits; ++i)
		result.push_back(std::pair<int, GLuint>(selectionBuffer[i * 4 + 3], selectionBuffer[i * 4 + 1]));

	return result;
}

// Draw a filled bounding box
void renderer_opengl_immediate::render_filled_bounding_box(const ldraw::metrics &metrics)
{
	const ldraw::vector &min = metrics.min_();
	const ldraw::vector &max = metrics.max_();
	
	glBegin(GL_QUADS);
	
	glVertex3f(max.x(), max.y(), min.z());
	glVertex3f(min.x(), max.y(), min.z());
	glVertex3f(min.x(), max.y(), max.z());
	glVertex3f(max.x(), max.y(), max.z());

	glVertex3f(max.x(), min.y(), max.z());
	glVertex3f(min.x(), min.y(), max.z());
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(max.x(), min.y(), min.z());

	glVertex3f(max.x(), max.y(), max.z());
	glVertex3f(min.x(), max.y(), max.z());
	glVertex3f(min.x(), min.y(), max.z());
	glVertex3f(max.x(), min.y(), max.z());

	glVertex3f(max.x(), min.y(), min.z());
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(min.x(), max.y(), min.z());
	glVertex3f(max.x(), max.y(), min.z());

	glVertex3f(min.x(), max.y(), max.z());
	glVertex3f(min.x(), max.y(), min.z());
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(min.x(), min.y(), max.z());

	glVertex3f(max.x(), max.y(), min.z());
	glVertex3f(max.x(), max.y(), max.z());
	glVertex3f(max.x(), min.y(), max.z());
	glVertex3f(max.x(), min.y(), min.z());
	
	glEnd();
}

// Draw a bounding box
void renderer_opengl_immediate::render_bounding_box(const ldraw::metrics &metrics)
{
	const ldraw::vector &min = metrics.min_();
	const ldraw::vector &max = metrics.max_();
	
	glBegin(GL_LINES);
	
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(max.x(), min.y(), min.z());
	
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(min.x(), max.y(), min.z());
	
	glVertex3f(min.x(), min.y(), min.z());
	glVertex3f(min.x(), min.y(), max.z());
	
	glVertex3f(max.x(), max.y(), max.z());
	glVertex3f(min.x(), max.y(), max.z());
	
	glVertex3f(max.x(), max.y(), max.z());
	glVertex3f(max.x(), min.y(), max.z());
	
	glVertex3f(max.x(), max.y(), max.z());
	glVertex3f(max.x(), max.y(), min.z());
	
	glVertex3f(max.x(), min.y(), max.z());
	glVertex3f(max.x(), min.y(), min.z());
	
	glVertex3f(min.x(), max.y(), max.z());
	glVertex3f(min.x(), max.y(), min.z());
	
	glVertex3f(max.x(), max.y(), min.z());
	glVertex3f(max.x(), min.y(), min.z());
	
	glVertex3f(min.x(), max.y(), max.z());
	glVertex3f(min.x(), min.y(), max.z());
	
	glVertex3f(max.x(), min.y(), max.z());
	glVertex3f(min.x(), min.y(), max.z());
	
	glVertex3f(max.x(), max.y(), min.z());
	glVertex3f(min.x(), max.y(), min.z());
	
	glEnd();
}

// Draw a line
void renderer_opengl_immediate::render_line(const ldraw::element_line &l)
{
	const unsigned char *color = get_color(l.get_color());
	glColor4ubv(color);

	glBegin(GL_LINES);
	glVertex3fv(l.pos1().get_pointer());
	glVertex3fv(l.pos2().get_pointer());
	glEnd();

	++m_stats.lines;
}

// Draw a triangle
void renderer_opengl_immediate::render_triangle(const ldraw::element_triangle &l)
{
	const unsigned char *color = get_color(l.get_color());
	glColor4ubv(color);

	glBegin(GL_TRIANGLES);
	glVertex3fv(l.pos1().get_pointer());
	glVertex3fv(l.pos2().get_pointer());
	glVertex3fv(l.pos3().get_pointer());
	
	glEnd();

	++m_stats.triangles;
	++m_stats.faces;
}

// Draw a quadrilateral
void renderer_opengl_immediate::render_quadrilateral(const ldraw::element_quadrilateral &l)
{
	const unsigned char *color = get_color(l.get_color());
	glColor4ubv(color);

	glBegin(GL_QUADS);
	glVertex3fv(l.pos1().get_pointer());
	glVertex3fv(l.pos2().get_pointer());
	glVertex3fv(l.pos3().get_pointer());
	glVertex3fv(l.pos4().get_pointer());
	glEnd();

	++m_stats.quads;
	++m_stats.faces;
}

// Draw a conditional line
// Draw v1v2 if the projections of both v1v2 and v3v4 are parallel
void renderer_opengl_immediate::render_condline(const ldraw::element_condline &l, const ldraw::matrix &projectionMatrix)
{
	const unsigned char *color = get_color(l.get_color());
	glColor4ubv(color);

	// Linear transform
	ldraw::vector tv1 = projectionMatrix * l.pos1();
	ldraw::vector tv2 = projectionMatrix * l.pos2();
	ldraw::vector tv3 = projectionMatrix * l.pos3();
	ldraw::vector tv4 = projectionMatrix * l.pos4();
	
	// Turn them into direction vectors
	ldraw::vector v1 = ldraw::vector(std::fabs(tv2.x()-tv1.x()), std::fabs(tv2.y()-tv1.y()), 0.0f).normalize();
	ldraw::vector v2 = ldraw::vector(std::fabs(tv4.x()-tv3.x()), std::fabs(tv4.y()-tv3.y()), 0.0f).normalize();
	
	// If parallel (approximately)
	if (fabs(v2.x()-v1.x()) <= LDR_EPSILON && fabs(v2.y()-v1.y()) <= LDR_EPSILON) {
		glBegin(GL_LINES);
		glVertex3fv(l.pos1().get_pointer());
		glVertex3fv(l.pos2().get_pointer());
		glEnd();

		++m_stats.lines;
	}
}

void renderer_opengl_immediate::render_stud(ldraw::model *l, bool edges)
{
	// Stud is the most frequently rendered part on every models.
	// And it is quite complex to render, causing major speed drop during entire model rendering process.
	// Therefore, replacing stud geometry to simpler primitive (such as line, square) makes rendering faster.

	glPushAttrib(GL_ENABLE_BIT);
	
	if (m_params->get_shading())
		glDisable(GL_LIGHTING);
	
	switch (m_params->get_stud_rendering_mode()) {
		case parameters::stud_regular:
			// Render stud as normal
			if (edges)
				draw_model_edges(l->parent(), l, -1, 0L);
			else
				draw_model_full(l->parent(), l, -1, 0L);
			return;
			
		case parameters::stud_line:
			// Draw a line
			glColor4ubv(m_colorstack.top().get_entity()->complement);
			glBegin(GL_LINES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, -4.0f, 0.0f);
			glEnd();
			return;
			
		case parameters::stud_square:
			// Draw a square
			glColor4ubv(m_colorstack.top().get_entity()->complement);
			glBegin(GL_LINES);
			glVertex3f(-6.0f, -4.0f, -6.0f);
			glVertex3f(6.0f, -4.0f, -6.0f);
			
			glVertex3f(6.0f, -4.0f, -6.0f);
			glVertex3f(6.0f, -4.0f, 6.0f);
			
			glVertex3f(6.0f, -4.0f, 6.0f);
			glVertex3f(-6.0f, -4.0f, 6.0f);
			
			glVertex3f(-6.0f, -4.0f, 6.0f);
			glVertex3f(-6.0f, -4.0f, -6.0f);
			glEnd();
			return;
	}

	glPopAttrib();
}

void renderer_opengl_immediate::render_normal_orientation(const ldraw::element_base *el, const ldraw::vector &nv, bool isccw)
{
	if (el->get_type() != ldraw::type_triangle && el->get_type() != ldraw::type_quadrilateral)
		return;

	ldraw::vector center;
	ldraw::vector nv3 = nv * 3.0f;

	if (el->get_type() == ldraw::type_triangle) {
		const ldraw::element_triangle *t = CAST_AS_CONST_TRIANGLE(el);

		center.x() = (t->pos1().x() + t->pos2().x() + t->pos3().x()) / 3.0f;
		center.y() = (t->pos1().y() + t->pos2().y() + t->pos3().y()) / 3.0f;
		center.z() = (t->pos1().z() + t->pos2().z() + t->pos3().z()) / 3.0f;
	} else {
		const ldraw::element_quadrilateral *t = CAST_AS_CONST_QUADRILATERAL(el);

		center.x() = (t->pos1().x() + t->pos2().x() + t->pos3().x() + t->pos4().x()) / 4.0f;
		center.y() = (t->pos1().y() + t->pos2().y() + t->pos3().y() + t->pos4().y()) / 4.0f;
		center.z() = (t->pos1().z() + t->pos2().z() + t->pos3().z() + t->pos4().z()) / 4.0f;
	}

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);

	glPushMatrix();
	glTranslatef(center.x(), center.y(), center.z());
	glBegin(GL_LINES);
	glColor3ub(255, 255, 255);
	glVertex3f(0.0f, 0.0f, 0.0f);

	if (isccw)
		glColor3ub(255, 0, 0);
	else
		glColor3ub(0, 0, 255);
	glVertex3fv(nv3.get_pointer());
	glEnd();
	glPopMatrix();

	glPopAttrib();
}

}
