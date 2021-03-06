#ifndef __FL_IMAGE_Repeat_Button_H
#define __FL_IMAGE_Repeat_Button_H
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl.H>

class Fl_Image_Repeat_Button: public Fl_Repeat_Button {
private:
	Fl_Image* upimage;
	Fl_Image* downimage;
public:
	Fl_Image* up_image();
	Fl_Image* down_image();
	void up_image(Fl_Image* p_up_image);
	void down_image(Fl_Image* p_down_image);
	void draw();
	Fl_Image_Repeat_Button(int a,int b,int c,int d,const char* e);
	Fl_Image_Repeat_Button(int a,int b,int c,int d);
};

#endif
