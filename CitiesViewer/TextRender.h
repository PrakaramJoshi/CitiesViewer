#pragma once
#include <freetype-gl.h>
#include <font-manager.h>
#include <vertex-buffer.h>
#include <text-buffer.h>
#include <markup.h>
#include <shader.h>
#include <mat4.h>
#include <string>
#include <mutex>
class TextRender
{

	// ------------------------------------------------------- global variables ---
	text_buffer_t * m_text_buffer;
	vec2 m_pen;
	markup_t m_markup;
	mat4   m_model, m_view, m_projection;
	std::mutex m_mutex;
	void setup_other_text();



	// ---------------------------------------------------------------- display ---
public:
	TextRender();

	
	void display(const int _viewPortWidth, 
				const int _viewPortHeight);


	void print(const std::wstring &_text,
		const float &_x,
		const float &_y);

	void test();

	~TextRender();
};

