#include "VideoWriter.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write
#include <DataStructure.h>
#include <Logger.h>

VideoWriter::VideoWriter()
{
	m_writer = 0;
	m_write_thread = nullptr;
	m_use_pbo = false;
}

VideoWriter::~VideoWriter()
{
	if (m_write_thread){
		m_frame_q.ShutDown();
		m_write_thread->join();
		delete m_write_thread;
		m_write_thread = nullptr;
	}
}

bool VideoWriter::setupVideoWriting(const std::string &_output_dir,
								const size_t &_width,
								const size_t &_height,
								bool _use_pbo){
	m_isColor = 1;
	m_fps = 59;
	m_width = _width;
	m_height = _height;
	// You can change CV_FOURCC('' to CV_FOURCC('X', '2', '6', '4') for x264 output
	// Make sure you compiled ffmpeg with x264 support and OpenCV with x264
	m_writer = new cv::VideoWriter();// cvCreateVideoWriter((_output_dir + "\\out.avi").c_str(), CV_FOURCC('M', 'J', 'P', 'G'), m_fps, cvSize(m_width, m_height), m_isColor);
	int ex = CV_FOURCC('M', 'J', 'P', 'G');     // Get Codec Type- Int form
	m_writer->open((_output_dir + "\\out.avi").c_str(), ex , m_fps, cvSize(m_width, m_height), true);
	if (!m_writer->isOpened())
	{
		AceLogger::Log("Could not open the output video for write: ", AceLogger::LOG_ERROR);
		return false;
	}
	m_write_thread = new std::thread(&VideoWriter::write_frame,this, &m_frame_q);
	m_use_pbo = _use_pbo;
	if (_use_pbo){
		unsigned int total_count = m_width * m_height * 3;
		glGenBuffers(1, &m_pbo);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
		glBufferData(GL_PIXEL_PACK_BUFFER, total_count*sizeof(char), 0, GL_DYNAMIC_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}
	
	return true;
	
}

void VideoWriter::write_frame(BlockingQueue<VideoWriter::FramePkt> *_frame_q){

	VideoWriter::FramePkt *frame_pkt= nullptr;
	
	while (_frame_q->Remove(&frame_pkt)){
		IplImage *img = cvCreateImage(cvSize(frame_pkt->p_width, frame_pkt->p_height), IPL_DEPTH_8U, 3);
		img->imageData = frame_pkt->p_framedata;
		cv::Mat img_mat(img);
		cvFlip(img);
		m_writer->write(img_mat);
		cvReleaseImage(&img);
		unsigned int count = frame_pkt->p_width*frame_pkt->p_height * 3;
		m_mem.add_to_cache_array(frame_pkt->p_framedata, count);
		delete frame_pkt;
	}
}

void VideoWriter::capture_frame(){
	if (m_use_pbo)
		capture_frame_pbo();
	else
		capture_frame_glread();
}

void VideoWriter::capture_frame_glread(){
	
	VideoWriter::FramePkt* frame_pkt = new VideoWriter::FramePkt();
	unsigned int total_count = m_width * m_height * 3;
	frame_pkt->p_framedata = m_mem.get_mem_array(total_count);
	frame_pkt->p_height = m_height;
	frame_pkt->p_width = m_width;
	glReadPixels(0, 0, m_width, m_height, GL_BGR, GL_UNSIGNED_BYTE, frame_pkt->p_framedata);
	m_frame_q.Insert(frame_pkt);

}

void VideoWriter::capture_frame_pbo(){
	
	unsigned int total_count = m_width * m_height * 3;
	VideoWriter::FramePkt* frame_pkt = new VideoWriter::FramePkt();
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo);
	glReadPixels(0, 0, m_width, m_height, GL_BGR, GL_UNSIGNED_BYTE, 0);
	GLubyte *ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, total_count, GL_MAP_READ_BIT);
	frame_pkt->p_framedata = m_mem.get_mem_array(total_count);
	memcpy(frame_pkt->p_framedata, ptr, total_count);
	frame_pkt->p_height = m_height;
	frame_pkt->p_width = m_width;
	m_frame_q.Insert(frame_pkt);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

}

void VideoWriter::close_and_save(){
	m_frame_q.ShutDown();
	if (m_write_thread)
		m_write_thread->join();
	delete m_write_thread;
	m_write_thread = nullptr;
	AceLogger::Log("total mem blocks reused : " + StringUtils::get_string(m_mem.get_reuse_count()));
	AceLogger::Log("total mem blocks new    : " + StringUtils::get_string(m_mem.get_new_count()));

}