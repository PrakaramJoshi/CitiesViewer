#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H
#include <string>
#include <vector>
#include <boost\program_options.hpp>
#include <Logger\Logger.h>
#include <boost\filesystem.hpp>
#include <boost\algorithm\string.hpp>
#include <Geography\Cities.h>



struct app_settings{
	PROJECTION p_projection;
	float p_circle_radius_max;
	float p_circle_radius_min;
	bool p_filled_circles;
	unsigned long p_poll_delay;
	bool p_use_ipcQ;
	std::string p_cities_database;
	std::string p_white_list_names;
	std::string p_black_list_names;
	std::vector<float> p_start_Color;
	std::vector<float> p_end_color;
	float p_rotation_speed;
	float p_rotation_height;

	float p_camera_up_x;
	float p_camera_up_y;
	float p_camera_up_z;

	int  p_read_city_count;
	int p_globe_rings;
	int p_globe_sectors;

	bool p_record_video;

	app_settings(){
		p_projection = PROJECTION::TOTAL_PROJECTIONS;
		p_circle_radius_max = 0.09;
		p_circle_radius_min = 0.01;
		p_filled_circles = false;
		p_poll_delay = 1000;
		p_use_ipcQ = false;
		p_cities_database = "";
		p_white_list_names = "";
		p_black_list_names = "";

		p_start_Color.push_back(1.0);
		p_start_Color.push_back(0.0);
		p_start_Color.push_back(0.0);

		p_end_color.push_back( 0.0);
		p_end_color.push_back(0.0);
		p_end_color.push_back(0.0);

		p_rotation_speed = GeoGraphyUtils::DTR(36);
		p_rotation_height = (GeoGraphyUtils::earth_radius + 12000) / GeoGraphyUtils::earth_radius;
		p_camera_up_x = 0;
		p_camera_up_y = 0;
		p_camera_up_z = 0;

		p_read_city_count = -1;

		p_globe_rings = 180;
		p_globe_sectors = 360;

		p_record_video = false;
	};

	bool read_settings(const std::string &_settings_file){
		try
		{
			std::vector<std::string> options;
			if (!read_file(_settings_file, options)){
				AceLogger::Log("Cannot read settings as the settings file not found.", AceLogger::LOG_ERROR);
				return false;
			}

			std::string all_projections = "map projection (" + projection_str[0];
			for (int i = 1; i < PROJECTION::TOTAL_PROJECTIONS; i++){
				all_projections = all_projections + "," + projection_str[i];
			}
			all_projections = all_projections + ")";

			namespace po = boost::program_options;
			po::options_description desc("Options");
			std::string projection_val;
			std::string mesh_outline;
			std::string selected_city_data;
			std::string gradient_end; 
			std::string gradient_start;


			float rotation_speed;
			float rotation_height;


			std::string read_city_count;
			std::string record_video;

			desc.add_options()
				("help", "Print help messages")
				("projection", po::value<std::string>(&projection_val)->default_value(projection_str[PROJECTION::GALL_PETERS].c_str()), all_projections.c_str())
				("radius_max", po::value<float>(&p_circle_radius_max)->default_value(0.09), "maximum circle radius")
				("radius_min", po::value<float>(&p_circle_radius_min)->default_value(0.01), "minimum circle radius")
				("circle_fill", po::value<std::string>(&mesh_outline)->default_value("outline"), "circle fill type(outline/fill)")
				("gradient_end", po::value<std::string>(&gradient_end)->default_value("0.0,0.0,0.0"), "outer color r,g,b (0 to 1)")
				("gradient_start", po::value<std::string>(&gradient_start)->default_value("1.0,0.0,0.0"), "inner color r,g,b (0 to 1)")
				("rotation_speed", po::value<float>(&rotation_speed)->default_value(36), "rotations for spherical projection in degrees per second")
				("rotation_height", po::value<float>(&rotation_height)->default_value(12000), "height above earth surface in KM for the camera")
				("camera_up_x", po::value<float>(&p_camera_up_x)->default_value(0), "camera up x")
				("camera_up_y", po::value<float>(&p_camera_up_y)->default_value(0), "camera up y")
				("camera_up_z", po::value<float>(&p_camera_up_z)->default_value(0), "camera up z")
				("poll_delay", po::value<unsigned long>(&p_poll_delay)->default_value(1000), "poll delay in ms")
				("select_city", po::value<std::string>(&selected_city_data)->default_value("random"), "select cities(random/ipcQ)")
				("city_database", po::value<std::string>(&p_cities_database)->default_value(""), "cities database with coordinates")
				("white_list", po::value<std::string>(&p_white_list_names)->default_value(""), "cities which can be selected")
				("black_list", po::value<std::string>(&p_black_list_names)->default_value(""), "cities which will NOT be selected")
				("read_city_count", po::value<std::string>(&read_city_count)->default_value("*"), "maximum number of cities to be read from the database( for all : *, for any other count : count)")
				("globe_rings", po::value<int>(&p_globe_rings)->default_value(180), "number of rings for the globe to be created(latitudes, more rings,more smooth globe)")
				("globe_sectors", po::value<int>(&p_globe_sectors)->default_value(360), "number of sectors for the globe to be created(longitudes, more rings,more smooth globe)")
				("reacord_video", po::value<std::string>(&record_video)->default_value("No"), "record video (Yes/No)");

			po::variables_map vm;
			try
			{
				po::store(po::command_line_parser(options).options(desc).run(),
					vm); 
				if (vm.count("help"))
				{
					AceLogger::Log("Help not available", AceLogger::LOG_WARNING);
					return false;
				}

				po::notify(vm); 
				p_use_ipcQ = use_ipcQ(selected_city_data);
				p_filled_circles = fill_circle(mesh_outline);
				p_projection = get_projection(projection_val);
				p_rotation_speed = GeoGraphyUtils::DTR(rotation_speed);
				p_rotation_height = (GeoGraphyUtils::earth_radius + rotation_height) / GeoGraphyUtils::earth_radius;

				if (!fill_gradient(gradient_start, p_start_Color)){
					return false;
				}
				if (!fill_gradient(gradient_end, p_end_color)){
					return false;
				}
				if (!get_read_city_count(read_city_count, p_read_city_count)){
					return false;
				}
				if (!get_bool(record_video, p_record_video)){
					return false;
				}
				return validate_settings();
			}
			catch (po::error& e)
			{
				AceLogger::Log(e.what(), AceLogger::LOG_ERROR);
				return false;
			}


		}
		catch (std::exception& e)
		{
			AceLogger::Log("Unhandled Exception", AceLogger::LOG_ERROR);
			AceLogger::Log(e.what(), AceLogger::LOG_ERROR);
			return false;

		}
	}
private:
	bool read_file(const std::string &_file_path,
		std::vector<std::string> &_data)const{
		if (!boost::filesystem::exists(boost::filesystem::path(_file_path))){
			AceLogger::Log("file doesn't exist: " + _file_path, AceLogger::LOG_ERROR);
			return false;
		}
		std::ifstream ifs(_file_path);
		std::string temp;
		while (std::getline(ifs, temp)){
			_data.push_back( temp);
		}
		ifs.close();
		return true;
	};

	bool fill_circle(const std::string &_str)const{
		return boost::iequals(_str, "fill");
	};

	bool use_ipcQ(const std::string &_str)const{
		return boost::iequals(_str, "ipcQ");
	};

	bool get_bool(std::string _str,bool &_val){
		boost::trim(_str);
		if (boost::iequals(_str, "Yes")){
			_val = true;
			return true;
		}
		else if (boost::iequals(_str, "No")){
			_val = false;
			return true;
		}
		return false;
			
	}

	bool fill_gradient(const std::string &_val,
						std::vector<float>&_c){
		std::vector<std::string>tokens;
		StringUtils::split(_val, tokens,',');
		if (tokens.size() != 3){
			AceLogger::Log("invalid info for color gradient", AceLogger::LOG_ERROR);
			return false;
		}
		if (!StringUtils::is_type<float>(tokens[0], _c[0])){
			AceLogger::Log("invalid data for r in color gradient", AceLogger::LOG_ERROR);
			return false;
		}
		if (!StringUtils::is_type<float>(tokens[1], _c[1])){
			AceLogger::Log("invalid data for g in color gradient", AceLogger::LOG_ERROR);
			return false;
		}
		if (!StringUtils::is_type<float>(tokens[2], _c[2])){
			AceLogger::Log("invalid data for b in color gradient", AceLogger::LOG_ERROR);
			return false;
		}
		return true;
	}

	bool get_read_city_count(std::string _val,
							 int &_read_city_count){
		boost::trim(_val);
		if (boost::iequals("*", _val)){
			_read_city_count = -1; 
			return true;
		}
		if (StringUtils::is_type<int>(_val, _read_city_count)){
			return true;
		}
		AceLogger::Log("Invalid city count: " + _val, AceLogger::LOG_ERROR);
		return false;
	}

	bool validate_settings()const {
		bool returnVal = true;
		if (!boost::filesystem::exists(boost::filesystem::path(p_cities_database))){
			AceLogger::Log("cities database file not found at " + p_cities_database, AceLogger::LOG_ERROR);
			returnVal = false;
		}

		if (!boost::filesystem::exists(boost::filesystem::path(p_white_list_names))){
			AceLogger::Log("white list file not found at " + p_white_list_names, AceLogger::LOG_ERROR);
			returnVal = false;
		}

		if (!boost::filesystem::exists(boost::filesystem::path(p_black_list_names))){
			AceLogger::Log("black list file not found at " + p_black_list_names, AceLogger::LOG_ERROR);
			returnVal = false;
		}

		if (p_projection == PROJECTION::TOTAL_PROJECTIONS){
			AceLogger::Log("invalid projection!", AceLogger::LOG_ERROR);
			returnVal = false;
		}
		if (returnVal)
			AceLogger::Log("input data validated");

		return returnVal;
	}
};
#endif