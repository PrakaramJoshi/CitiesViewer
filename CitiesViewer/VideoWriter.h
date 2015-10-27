#pragma once
#include<string>
#include <vector>
#include <DataStructure.h>
#include <thread>
#include <glew.h>
#include "MemoryCache.h"
namespace cv{
	class VideoWriter;
}
class VideoWriter
{
	struct FramePkt{
		char *p_framedata;
		unsigned int p_width;
		unsigned int p_height;
	};
	std::string m_output_dir;
	cv::VideoWriter  *m_writer;
	BlockingQueue<VideoWriter::FramePkt> m_frame_q;
	MemoryCache<char> m_mem;

	int m_isColor;
	int m_fps;
	int m_width;
	int m_height;
	std::thread *m_write_thread;
	
	GLuint m_pbo;
	bool m_use_pbo;

	void write_frame(BlockingQueue<VideoWriter::FramePkt> *_frame_q);

	void capture_frame_glread();

	void capture_frame_pbo();

public:
	VideoWriter();

	void capture_frame();

	bool setupVideoWriting(const std::string &_output_dir,
						const size_t &_width,
						const size_t &_height,
						bool _use_pbo);

	void close_and_save();

	~VideoWriter();
};

