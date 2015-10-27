// CitiesViewer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Logger.h"

#include <glfwWindow.h>
#include <Geography/Cities.h>
#include <ShadersUtility.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>


#include <StringUtils.h>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>

#include "VideoWriter.h"
#include "TextRender.h"
#include "Geometry\DrawingUtils.h"
#include "app_settings.h"

using namespace AceLogger;
REGISTER_LOGGER("B:\\Logs", "CitiesViewer", "0.0.0.1")

//typedef IPCBufferSocketUDP<std::string> IPCQ;
typedef IPCBufferSocketTCP<std::string> IPCQ;
//typedef IPCMessageQ<std::string> IPCQ;

vao_state m_city_points;
vao_state m_selected_cities;
vao_state m_globe;


DrawingUtils::PhongLighting m_phong;

GLuint m_programID;

DrawingUtils::SolidSphere *sphere;
DrawingUtils::Circle circle;
std::vector<float> m_points_selected;
std::vector<float> m_colours_selected;

int total_cities;
int selected_cities_count;
Cities cities;
SearchCities searchCities;
SearchCityNames searchcitiesNames;
app_settings m_app_settings;

std::mutex cities_mutex;
struct FoundCities{
	std::map<City, unsigned long> p_cities;
	unsigned int p_max_count;
	unsigned int p_min_count;
}typedef FoundCities;
BlockingQueue<FoundCities > selected_citiesQ(10);

float on_screen_x(const City &_c){
	switch (m_app_settings.p_projection){
	case PROJECTION::AZIMUTHAL:
		return GeoGraphyUtils::azimuthal_equidistant::x_glx(GeoGraphyUtils::azimuthal_equidistant::x(_c.longitude(), _c.latitude()));
		break;
	case PROJECTION::GALL_PETERS:
		return  GeoGraphyUtils::gall_peters::x_glx(GeoGraphyUtils::gall_peters::x(_c.longitude()));
		break;
	case PROJECTION::MERCATOR:
		return  GeoGraphyUtils::mercator::x_glx(GeoGraphyUtils::mercator::x(_c.longitude()));
		break;
	case PROJECTION::SPHERICAL:
		return GeoGraphyUtils::spherical::x_glx(GeoGraphyUtils::spherical::x(_c.longitude(), _c.latitude()));
		break;
	case PROJECTION::TOTAL_PROJECTIONS:
		Log("invalid projection", AceLogger::LOG_ERROR);
		break;
	default:
		Log("No valid projection defined", AceLogger::LOG_ERROR);
	}
	return 0;
}

float on_screen_y(const City &_c){
	switch (m_app_settings.p_projection){
	case PROJECTION::AZIMUTHAL:
		return GeoGraphyUtils::azimuthal_equidistant::y_gly(GeoGraphyUtils::azimuthal_equidistant::y(_c.longitude(), _c.latitude()));
		break;
	case PROJECTION::GALL_PETERS:
		return GeoGraphyUtils::gall_peters::y_gly(GeoGraphyUtils::gall_peters::y(_c.latitude()));
		break;
	case PROJECTION::MERCATOR:
		return GeoGraphyUtils::mercator::y_gly(GeoGraphyUtils::mercator::y(_c.latitude()));
		break;
	case PROJECTION::SPHERICAL:
		return GeoGraphyUtils::spherical::y_gly(GeoGraphyUtils::spherical::y(_c.longitude(), _c.latitude()));
		break;
	case PROJECTION::TOTAL_PROJECTIONS:
		Log("invalid projection", AceLogger::LOG_ERROR);
		break;
	default:
		Log("No valid projection defined", AceLogger::LOG_ERROR);
	}
	return 0;
}

float on_screen_z(const City &_c){
	switch (m_app_settings.p_projection){
	case PROJECTION::AZIMUTHAL:
		return 0;
		break;
	case PROJECTION::GALL_PETERS:
		return 0;
		break;
	case PROJECTION::MERCATOR:
		return 0;
		break;
	case PROJECTION::SPHERICAL:
		return GeoGraphyUtils::spherical::y_gly(GeoGraphyUtils::spherical::z(_c.latitude()));
		break;
	case PROJECTION::TOTAL_PROJECTIONS:
		Log("invalid projection", AceLogger::LOG_ERROR);
		break;
	default:
		Log("No valid projection defined", AceLogger::LOG_ERROR);
	}
	return 0;
}

void SetDataForShape(std::vector<float> &_points,
					 std::vector<float> &_colours){
	
	cities.read_cities_dataset(m_app_settings.p_cities_database, m_app_settings.p_read_city_count);
	std::vector<City> allCities;
	cities.cities(allCities);

	searchcitiesNames.add_cities(cities.countries(),allCities, m_app_settings.p_white_list_names, m_app_settings.p_black_list_names);

	searchCities.set_cities(allCities);

	const auto &all_cities = cities.cities();
	_points.resize(all_cities.size() * 3);
	_colours.resize(all_cities.size() * 3);

	int count = 0;
	total_cities = all_cities.size();

	for (auto iter = all_cities.begin(); iter != all_cities.end(); ++iter){

		_points[count] = on_screen_x(*iter);

		_colours[count] = 1.0f;
		count++;

		_points[count] = on_screen_y(*iter);
		
		_colours[count] = 1.0f;
		count++;
		_points[count] = on_screen_z(*iter);
		_colours[count] = 1.0f;
		count++;
	}
	Log("Completed loading cities info!");
}

void load_shaders(){

	if (m_programID)
		glDeleteProgram(m_programID);
	std::string shader_vs = R"(B:\Workspace\CitiesViewer\CitiesViewer\Shaders\vertexcolor_vs.glsl)";
	std::string shader_fs = R"(B:\Workspace\CitiesViewer\CitiesViewer\Shaders\vertexcolor_fs.glsl)";
	std::vector<std::pair<std::string, GLenum>> shaders;

	shaders.push_back(std::make_pair(shader_vs, GL_VERTEX_SHADER));
	shaders.push_back(std::make_pair(shader_fs, GL_FRAGMENT_SHADER));
	m_programID = GetProgramID_FileShaders(shaders);

	m_phong.setup(m_programID);
	CHECK_GL_ERROR
}

bool Setup(){

	// Dark blue background
	CHECK_GL_ERROR
	if (m_app_settings.p_projection==PROJECTION::SPHERICAL)
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	else
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	CHECK_GL_ERROR

	load_shaders();

	// prepare the game , load resources
	std::vector<float> points, colors;
	SetDataForShape(points,colors);

	generate_gl_buffers(m_city_points, total_cities * 3, total_cities * 3, &points[0], &colors[0]);

	m_points_selected.push_back(1);
	m_colours_selected.push_back(1);
	generate_gl_buffers(m_selected_cities, 0, 0, &m_points_selected[0], &m_colours_selected[0]);

	if (m_app_settings.p_projection == PROJECTION::SPHERICAL){
		generate_gl_buffers(m_globe, sphere->size(), sphere->size(), sphere->vertices_array(), sphere->colors_array());
		sphere->send_data_to_gpu(m_globe.p_vertexbuffer, m_globe.p_colours_vbo, m_globe.p_vao);
	}
	sphere->release_memory();
	
	CHECK_GL_ERROR
	glBindVertexArray(m_city_points.p_vao);
	glUseProgram(m_programID);

	CHECK_GL_ERROR
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	// Get a handle for our "MVP" uniform
	//m_MatrixID = glGetUniformLocation(m_programID, "MVP");
	return true;
}

void add_bar(float *_points,
	float *_colours,
	std::vector<float> &_start,
	const float &_scale){

	_points[0] = _start[0];
	_points[1] = _start[1];
	_points[2] = _start[2];

	_points[3] = _start[0]*_scale;
	_points[4] = _start[1]*_scale;
	_points[5] = _start[2]*_scale;

	_colours[0] = m_app_settings.p_start_Color[0];
	_colours[1] = m_app_settings.p_start_Color[1];
	_colours[2] = m_app_settings.p_start_Color[2];

	_colours[3] = m_app_settings.p_end_color[0];
	_colours[4] = m_app_settings.p_end_color[1];
	_colours[5] = m_app_settings.p_end_color[2];

}

void get_random_cities(FoundCities*&_cities){
	std::chrono::milliseconds interval(m_app_settings.p_poll_delay);
	auto selected_cities = cities.get_random(0.0001,_cities->p_max_count,_cities->p_min_count);
	_cities->p_cities.swap(selected_cities);
	std::this_thread::sleep_for(interval);
}

void get_citied_from_ipcQ(IPCQ &_ipcQ,
							FoundCities*&_cities){
	auto time_now = std::chrono::high_resolution_clock::now();
	auto time_later = std::chrono::high_resolution_clock::now();
	auto duration2 = std::chrono::duration<double, std::milli>(time_later - time_now).count();
	std::string *msg;
	while (_ipcQ.Receive_try(&msg) && duration2<m_app_settings.p_poll_delay){
		if (msg){
			unsigned int max_hit = 0;
			unsigned int min_hit = 0;
			searchcitiesNames.find(_cities->p_cities, max_hit, min_hit, *msg);
			if (max_hit>_cities->p_max_count)
				_cities->p_max_count = max_hit;
			if (min_hit<_cities->p_min_count)
				_cities->p_min_count = min_hit;
			delete msg;
			time_later = std::chrono::high_resolution_clock::now();
			duration2 = std::chrono::duration<double, std::milli>(time_later - time_now).count();
		}
	}
}

void print_selected_city_stats(FoundCities*&_cities){
	Log("Found total of " + StringUtils::get_string(_cities->p_cities.size()) + "cities");
	std::vector<std::pair<City, unsigned long> > all_cities_found;
	for (auto iter = _cities->p_cities.begin(); iter != _cities->p_cities.end(); ++iter){
		all_cities_found.push_back(std::make_pair((*iter).first, (*iter).second));
	}
	std::sort(all_cities_found.begin(), all_cities_found.end(), [](const std::pair<City, unsigned long> &_a,
		const std::pair<City, unsigned long> &_b)->bool{
		return _a.second > _b.second;
	});

	Log("Top most menitoned cities ");
	int top_cities = 40;
	int count_mentioned = 1;
	int show_names_on_screen = 5;
	for (auto iter = all_cities_found.begin(); iter != all_cities_found.end() && count_mentioned < top_cities; ++iter){
		Log(StringUtils::get_string(count_mentioned) + ")\t" + (*iter).first.name() + "," + cities.country_name((*iter).first) + "\t\t............\t" + StringUtils::get_string((*iter).second));
		/*if (count_mentioned < show_names_on_screen){
			std::wstring name((*iter).first.name().begin(), (*iter).first.name().end());
			_text_renderer->print(name, 10, 40+(10*count_mentioned));
		}*/
		count_mentioned++;
	}
}

void select_cities(IPCQ &_ipcQ/*,
					TextRender *_text_renderer*/){
	
	while (selected_citiesQ.CanInsert()){
		
		auto * c = new FoundCities();
		c->p_max_count = 0;
		c->p_min_count = 0;
		if (m_app_settings.p_use_ipcQ)
			get_citied_from_ipcQ(_ipcQ,c);
		else
			get_random_cities(c);
		if (c->p_cities.size() > 0){
			print_selected_city_stats(c/*, _text_renderer*/);
			selected_citiesQ.Insert(c);
		}
		else
			delete c;
	}	
}

int get_points_per_city(){
	if (m_app_settings.p_projection == PROJECTION::SPHERICAL){
		return 2;
	}
	return circle.get_points_count_on_circle();
}

int get_size_per_selected_city(){
	
	return get_points_per_city() * 3;
}

int get_total_selected_size(){
	return selected_cities_count * get_size_per_selected_city();
}

void fill_selected_cities(){

	FoundCities *selected_cities;

	while (selected_citiesQ.Remove(&selected_cities)){
		if (!selected_cities){
			continue;
		}
		cities_mutex.lock();
	
		selected_cities_count = (*selected_cities).p_cities.size();
		if (selected_cities_count == 0){
			delete selected_cities;
			cities_mutex.unlock();
			continue;
		}

		m_colours_selected.resize(get_total_selected_size());
		m_points_selected .resize(get_total_selected_size());

		int selection_count = 0;
		double scale = (m_app_settings.p_circle_radius_max-m_app_settings.p_circle_radius_min)/(selected_cities->p_max_count - selected_cities->p_min_count + 1);
		std::vector<float> center;
		center.push_back(0.0);
		center.push_back(0.0);
		center.push_back(0.0);
		for (auto iter = selected_cities->p_cities.begin(); iter != selected_cities->p_cities.end(); ++iter){
			auto radius = (1 + (*iter).second - selected_cities->p_min_count)*scale;
			radius += m_app_settings.p_circle_radius_min;
			center[0]=on_screen_x((*iter).first);
			center[1]=on_screen_y((*iter).first);
			center[2]=on_screen_z((*iter).first);
			if (m_app_settings.p_projection == SPHERICAL){
				radius *= 10;
				add_bar(&m_points_selected[0] + selection_count, &m_colours_selected[0] + selection_count,center, (1+radius));
			}
			else{
				circle.get_circle(&m_points_selected[0] + selection_count, &m_colours_selected[0] + selection_count, center, radius);
			}
			selection_count += get_size_per_selected_city();
		}
		delete selected_cities;
		cities_mutex.unlock();
	}
}

void set_view_ports_size(GLFWwindow *_window,
						ViewPort &_viewPort){
	
	glfwGetFramebufferSize(_window, &_viewPort.p_width, &_viewPort.p_height);
	_viewPort.p_start_x = 0;
	_viewPort.p_start_y = 0;
	
	if (_viewPort.p_height < _viewPort.p_width){
		_viewPort.p_start_x = (_viewPort.p_width - _viewPort.p_height) / 2;
		_viewPort.p_width = _viewPort.p_height;
	}
	else{
		_viewPort.p_start_y = (_viewPort.p_height - _viewPort.p_width) / 2;
		_viewPort.p_height = _viewPort.p_width;
	}

	glViewport(_viewPort.p_start_x, _viewPort.p_start_y, _viewPort.p_width, _viewPort.p_height);
}

void on_mouse_change_position(GLFWwindow *_window,
							const ViewPort &_viewPort,
							TextRender &_text_render,
							double &_xpos_previous,
							double &_ypos_previous){

	if (m_app_settings.p_projection == PROJECTION::SPHERICAL){
		// TOOD currently only x and y are mapped,
		// until z is mapped back
		// the nearest city search is not 
		// available in the spherical projection
		return;
	}
	double xpos, ypos;
	glfwGetCursorPos(_window, &xpos, &ypos);
	xpos -= _viewPort.p_start_x;
	ypos -= _viewPort.p_start_y;
	if (xpos != _xpos_previous || ypos != _ypos_previous){

		_xpos_previous = xpos;
		_ypos_previous = ypos;

		float x = (xpos * 2) / _viewPort.p_width, y = (ypos * 2) / _viewPort.p_height;
		// the  coordinates are from 0 to 2, 
		// we need to change them to -1 to 1 
		// also y needs to be flipped
		x = x - 1;
		y = -y + 1;
		float latidue, longitude;
		switch (m_app_settings.p_projection){
		case PROJECTION::AZIMUTHAL:
			x = GeoGraphyUtils::azimuthal_equidistant::glx_x(x);
			y = GeoGraphyUtils::azimuthal_equidistant::gly_y(y);
			GeoGraphyUtils::azimuthal_equidistant::coordinate(latidue, longitude, x, y);
			break;
		case PROJECTION::GALL_PETERS:
			x = GeoGraphyUtils::gall_peters::glx_x(x);
			y = GeoGraphyUtils::gall_peters::gly_y(y);
			GeoGraphyUtils::gall_peters::coordinate(latidue, longitude, x, y);
			break;
		case PROJECTION::MERCATOR:
			x = GeoGraphyUtils::mercator::glx_x(x);
			y = GeoGraphyUtils::mercator::gly_y(y);
			GeoGraphyUtils::mercator::coordinate(latidue, longitude, x, y);
			break;
		case PROJECTION::SPHERICAL:
			x = GeoGraphyUtils::spherical::glx_x(x);
			y = GeoGraphyUtils::spherical::gly_y(y);
			GeoGraphyUtils::spherical::coordinate(latidue, longitude, x, y);
			break;
		case PROJECTION::TOTAL_PROJECTIONS:
			Log("invalid projection", AceLogger::LOG_ERROR);
			break;
		default:
			Log("No valid projection defined", AceLogger::LOG_ERROR);
		}

		auto closest = searchCities.FindClosest(latidue, longitude);
		auto cityname = closest.name();
		std::wstring wcityName(cityname.begin(), cityname.end());
		_text_render.print(GeoGraphyUtils::Bearing(latidue, longitude) + L" Closest City : " + wcityName, 10, 40);
	}
}

#define DRAW_TYPE GL_LINE_LOOP

#define INSTANCED_DRAW \
	glDrawArraysInstanced(DRAW_TYPE, 0, circle_points, _previous_count);

#define LOOPED_DRAW \
for (int i = 0; i < _previous_count; i++) \
	glDrawArrays(m_app_settings.p_filled_circles ? GL_TRIANGLES : DRAW_TYPE, i*circle_points*fill_factor, circle_points*fill_factor);

#define DRAW_CIRCLES LOOPED_DRAW

void draw_selected_city_geo(const int _previous_count){
	auto render_type = GL_TRIANGLES;
	
	if (m_app_settings.p_projection == PROJECTION::SPHERICAL){
		render_type = GL_LINES;
	}
	else{
		render_type = m_app_settings.p_filled_circles ? GL_TRIANGLES : GL_LINE_LOOP;
	}
	for (int i = 0; i < _previous_count; i++) \
		glDrawArrays(render_type, i*get_points_per_city(), get_points_per_city());
}

void render_selected_cities(int &_previous_count){

	glBindVertexArray(m_selected_cities.p_vao);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (cities_mutex.try_lock()){
		if (selected_cities_count > 0){
			_previous_count = selected_cities_count;
			glBindBuffer(GL_ARRAY_BUFFER, m_selected_cities.p_vertexbuffer);

			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* get_total_selected_size(), &m_points_selected[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, m_selected_cities.p_colours_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*get_total_selected_size(), &m_colours_selected[0], GL_STATIC_DRAW);

			draw_selected_city_geo(_previous_count);
			selected_cities_count = 0;
		}
		else{
			draw_selected_city_geo(_previous_count);
		}
		cities_mutex.unlock();
	}
	else{
		draw_selected_city_geo(_previous_count);
	}
}

void display_globe()
{

#ifdef DRAW_WIREFRAME
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
	if (m_app_settings.p_projection != PROJECTION::SPHERICAL)
		return;
	sphere->draw( m_globe.p_vao);

}

void render(GLFWwindow *_window,
			double &_xpos_previous,
			double &_ypos_previous,
			int &_previous_count,
			TextRender &_text_render,
			DrawingUtils::camera<DrawingUtils::camera_motions::rotate_about_z> &_camera,
			ViewPort &_viewPort){
	glUseProgram(m_programID);
	
	set_view_ports_size(_window, 
						_viewPort);

	on_mouse_change_position(_window, 
							_viewPort,
							 _text_render, 
							 _xpos_previous, 
							 _ypos_previous);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera for the view
	glEnable(GL_DEPTH_TEST);

	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 projection;
	glm::vec3 camera_pos;
	_camera.move(_viewPort.p_width, _viewPort.p_height, view, model, projection);
	camera_pos = { _camera.get_current_x(), _camera.get_current_y(), _camera.get_current_z() };

	m_phong.render_lighting(view, model, projection, camera_pos);

	// render the all the world cities from the database
	display_globe();

	glBindVertexArray(m_city_points.p_vao);
	glDrawArrays(
		GL_POINTS, 0, total_cities);

	// render the selected cities 
	render_selected_cities(_previous_count);

	//render the text
	if (m_app_settings.p_projection == PROJECTION::SPHERICAL){
		std::wstring camera_at = L"Camera looking at x: " + StringUtils::get_wstring(_camera.get_look_at_x()) + L", y: " + StringUtils::get_wstring(_camera.get_look_at_y()) + L", z: " + StringUtils::get_wstring(_camera.get_look_at_z());
		
		std::wstring camera_up = L" up x: " + StringUtils::get_wstring(_camera.get_up_x()) + L", y: " + StringUtils::get_wstring(_camera.get_up_y()) + L", z: " + StringUtils::get_wstring(_camera.get_up_z());
		std::wstring camera_current = L" Current x: " + StringUtils::get_wstring(_camera.get_current_x()) + L", y: " + StringUtils::get_wstring(_camera.get_current_y()) + L", z: " + StringUtils::get_wstring(_camera.get_current_z());
		_text_render.print(camera_at + camera_up + camera_current, 10, 40);
	}

	_text_render.display(_viewPort.p_width, 
						_viewPort.p_height);
	
}
void init_reference_circle(){
	
	circle.set_circle_resolution(360);
	circle.set_data_per_vertex_points(3);
	circle.set_end_colors(m_app_settings.p_end_color);
	circle.set_is_filled(m_app_settings.p_filled_circles);
	circle.set_radius(1);
	circle.set_start_colors(m_app_settings.p_start_Color);
	circle.update_changes();
}

bool read_app_settings(){
	std::string app_setting_file = R"(B:\Workspace\CitiesViewer\app_settings.txt)";
	Log("Reading app settings file at : " + app_setting_file);
	return m_app_settings.read_settings(app_setting_file);
}

void exitApp(){
	// exit sequence 

	//close the logger
	FinishLog();
};

void clean_up(std::thread &_thread1,
			  std::thread &_thread2){
	Log_now("Clearing globe...");
	delete sphere;
	Log_now("Done!");

	Log_now("Shutting the select cities Q...");
	selected_citiesQ.ShutDown();
	Log_now("Done!");
	Log_now("Cleaning the select cities Q...");
	selected_citiesQ.CleanUp();
	Log_now("Done!");
	Log_now("Waiting on fill selected cities thread...");
	_thread1.join();
	Log_now("Done!");
	Log_now("Waiting on add selected cities thread...");
	_thread2.join();
	Log_now("Done!");
}

void ReloadShaders(){
	load_shaders();
}

void init_camera(DrawingUtils::camera<DrawingUtils::camera_motions::rotate_about_z> &_camera){

	float rotation_speed = m_app_settings.p_rotation_speed;
	float translation_speed = 0;
	float up_x = m_app_settings.p_camera_up_x;
	float up_y = m_app_settings.p_camera_up_y;
	float up_z = m_app_settings.p_camera_up_z;
	float initial_x = 0;
	float initial_y = m_app_settings.p_rotation_height;
	float initial_z = 0;
	float look_at_x = 0;
	float look_at_y = 0;
	float look_at_z = 0;
	float rotation_radius = m_app_settings.p_rotation_height;

	if (m_app_settings.p_projection != PROJECTION::SPHERICAL){
		rotation_speed = 0;
		up_x = 0;
		up_y = 1;
		up_z = 0;
		initial_y = 0;
		initial_z = 2;
		rotation_radius = 0;
	}

	_camera.set_rotation_speed(rotation_speed);
	_camera.set_translation_speed(translation_speed);
	_camera.set_up_x(up_x);
	_camera.set_up_y(up_y);
	_camera.set_up_z(up_z);
	_camera.set_current_x(initial_x);
	_camera.set_current_y(initial_y);
	_camera.set_current_z(initial_z);
	_camera.set_look_at_x(look_at_x);
	_camera.set_look_at_y(look_at_y);
	_camera.set_look_at_z(look_at_z);
	_camera.set_rotation_radius(rotation_radius);
}

void read_user_keys(GLFWwindow *_window,
	DrawingUtils::camera<DrawingUtils::camera_motions::rotate_about_z> &_camera){

	float pan_speed = 0.01;
	if (glfwGetKey(_window, GLFW_KEY_R) == GLFW_PRESS){
		ReloadShaders();
	}
	if (glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS){
		_camera.set_look_at_z(_camera.get_look_at_z() + pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS){
		_camera.set_look_at_z(_camera.get_look_at_z() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS){
		_camera.set_look_at_x(_camera.get_look_at_x() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS){
		_camera.set_look_at_x(_camera.get_look_at_x() + pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS){
		_camera.set_look_at_y(_camera.get_look_at_y() + pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS){
		_camera.set_look_at_y(_camera.get_look_at_y() - pan_speed);
	}

	if (glfwGetKey(_window, GLFW_KEY_Q) == GLFW_PRESS){
		_camera.set_up_x(_camera.get_up_x() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS){
		_camera.set_up_x(_camera.get_up_x() + pan_speed);
	}

	if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS){
		_camera.set_up_y(_camera.get_up_y() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS){
		_camera.set_up_y(_camera.get_up_y() + pan_speed);
	}

	if (glfwGetKey(_window, GLFW_KEY_Z) == GLFW_PRESS){
		_camera.set_up_z(_camera.get_up_z() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_X) == GLFW_PRESS){
		_camera.set_up_z(_camera.get_up_z() + pan_speed);
	}

	if (glfwGetKey(_window, GLFW_KEY_1) == GLFW_PRESS){
		_camera.set_current_x(_camera.get_current_x() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_2) == GLFW_PRESS){
		_camera.set_current_x(_camera.get_current_x() + pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_3) == GLFW_PRESS){
		_camera.set_current_y(_camera.get_current_y() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_4) == GLFW_PRESS){
		_camera.set_current_y(_camera.get_current_y() + pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_5) == GLFW_PRESS){
		_camera.set_current_z(_camera.get_current_z() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_6) == GLFW_PRESS){
		_camera.set_current_z(_camera.get_current_z() + pan_speed);
	}

	if (glfwGetKey(_window, GLFW_KEY_7) == GLFW_PRESS){
		_camera.set_rotation_radius(_camera.get_rotation_radius() - pan_speed);
	}
	if (glfwGetKey(_window, GLFW_KEY_8) == GLFW_PRESS){
		_camera.set_rotation_radius(_camera.get_rotation_radius() + pan_speed);
	}


	if (glfwGetKey(_window, GLFW_KEY_C) == GLFW_PRESS){
		init_camera(_camera);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	atexit(exitApp);
	m_programID = 0;
	if (!read_app_settings()){
		Log("Cannot continue as unable to read settings file.", LOG_ERROR);
		exit(0);
	}
	
	sphere = new DrawingUtils::SolidSphere(1, m_app_settings.p_globe_rings,m_app_settings.p_globe_sectors);
	init_reference_circle();

	//IPCQ ipcQ("Q1", false);
	IPCQ ipcQ("127.0.0.1",13, false);
	//IPCQ ipcQ("127.0.0.1", 1200, false);
	ViewPort view_port(0,0,640,480);

	GLFWwindow *window = CreateGlfwWindow("Cities Viewer " + ipcQ.get_protocol_name(), view_port.p_width, view_port.p_height);
	if (!window)
		exit(EXIT_FAILURE);
	if (!Setup())
		exit(EXIT_FAILURE);

	TextRender text_render;
	std::thread thread1(fill_selected_cities);
	std::thread thread2(select_cities, std::ref(ipcQ));

	auto previous_count = selected_cities_count;
	double xpos_previous=0, ypos_previous=0;

	auto camera_motion = DrawingUtils::camera_motions::rotate_about_z();
	DrawingUtils::camera<DrawingUtils::camera_motions::rotate_about_z> camera(camera_motion);
	init_camera(camera);
	camera.restart_clock();
	Log("Starting video recorder...");
	VideoWriter video_writer;
	std::string out_put_dir = R"(B:\Workspace\CitiesViewer\x64\video_out)";

	if (m_app_settings.p_record_video)
		video_writer.setupVideoWriting(out_put_dir, view_port.p_width, view_port.p_height,false);

	while (!glfwWindowShouldClose(window)) {

		//render the scene
		render(window,
			xpos_previous,
			ypos_previous,
			previous_count,
			text_render,
			camera,
			view_port);

		// capture the video to output file
		if (m_app_settings.p_record_video)
			video_writer.capture_frame();

		//other stuff : FPS, check for user input
		FPSStatus(window);
		glfwPollEvents();
		glfwSwapBuffers(window);
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}
		read_user_keys(window,camera);
	}

	// save the output video captured
	if (m_app_settings.p_record_video)
		video_writer.close_and_save();

	Log_now("Stopping the cities viewer...");
	ipcQ.ShutDown();
	clean_up(thread1, thread2);
	Log_now("Exiting...");
	// Close OpenGL window and terminate GLFW
	glfwTerminate();
	return 0;
}

