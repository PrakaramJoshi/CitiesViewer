#include "TextRender.h"
#include <fstream>
#include <stdio.h>
#include <wchar.h>
#include <mutex>

TextRender::TextRender()
{
	mat4_set_identity(&m_projection);
	mat4_set_identity(&m_model);
	mat4_set_identity(&m_view);
	setup_other_text();
}
// ---------------------------------------------------------------- display ---
void TextRender::display(const int _viewPortWidth,
							const int _viewPortHeight)
{

	//glClearColor(0.40, 0.40, 0.45, 1.00);
	//glClearColor(1.0, 1.0, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_mutex.lock();
	mat4_set_orthographic(&m_projection, 0, _viewPortWidth, 0, _viewPortHeight, -1, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(m_text_buffer->shader);
	{
		glUniformMatrix4fv(glGetUniformLocation(m_text_buffer->shader, "model"),
			1, 0, m_model.data);
		glUniformMatrix4fv(glGetUniformLocation(m_text_buffer->shader, "view"),
			1, 0, m_view.data);
		glUniformMatrix4fv(glGetUniformLocation(m_text_buffer->shader, "projection"),
			1, 0, m_projection.data);
		text_buffer_render(m_text_buffer);
	}
	m_mutex.unlock();
}

/* ----------------------------------------------------------------- print - */
void TextRender::print(const std::wstring &_text,
						const float &_x,
						const float &_y)
{
	m_mutex.lock();
	m_pen = { { _x, _y } };
	m_markup.font = font_manager_get_from_markup(m_text_buffer->manager, &m_markup);
	text_buffer_clear(m_text_buffer);
	text_buffer_add_text(m_text_buffer, &m_pen, &m_markup, const_cast<wchar_t*>(_text.c_str()), _text.size());
	m_mutex.unlock();
}

void TextRender::setup_other_text(){
	m_text_buffer = text_buffer_new(LCD_FILTERING_OFF);
	vec4 black = { { 0.0, 0.0, 0.0, 1.0 } };
	vec4 white = { { 1.0, 1.0, 1.0, 1.0 } };
	vec4 none = { { 1.0, 1.0, 1.0, 0.0 } };

	m_pen = { { 10.0, 480.0 } };
	m_markup.family = "fonts/VeraMono.ttf",
		m_markup.size = 15.0;
	m_markup.bold = 0;
	m_markup.italic = 0;
	m_markup.rise = 0.0;
	m_markup.spacing = 0.0;
	m_markup.gamma = 1.0;
	m_markup.foreground_color = white;
	m_markup.background_color = none;
	m_markup.underline = 0;
	m_markup.underline_color = black;
	m_markup.overline = 0;
	m_markup.overline_color = black;
	m_markup.strikethrough = 0;
	m_markup.strikethrough_color = black;
	m_markup.font = 0;
}

TextRender::~TextRender()
{

}
