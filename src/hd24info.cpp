#include <iostream>
#include <string>
#include <convertlib.h>
#include <hd24fs.h>

using namespace std;

void showsongs(hd24project* currentproj)
{
	int numsongs = currentproj->songcount();
	
	cout << "======================================================================" << endl;
	
	if (numsongs == 0)
	{
		cout << "      There are no songs in this project." << endl;
	}
	
	for (int i = 1; i <= numsongs; i++) 
	{
		hd24song currsong = *(currentproj->getsong(i));
		string* songname1 = new string("");
		cout << "  ";
		
		if (i < 10) 
		{
			cout << " ";
		}
		
		cout << i << ": ";

		string* currsname = currsong.songname();
		*songname1 += *currsname;
		
		cout << *(Convert::padright(*songname1, 20, " "));
		cout << *(currsong.display_duration()) << ", ";
		
		string* chans = Convert::int2str(currsong.logical_channels());
		chans = Convert::padleft(*chans,2," ");
		
		cout << *chans << "ch. " << currsong.samplerate() << " Hz ";
		
		delete(currsname);
		delete(songname1);
		
		cout << endl;
	}
}

void showprojects(hd24fs* currenthd24)
{
	int numprojs = currenthd24->projectcount();

	for (int i = 1; i <= numprojs; i++) 
	{
		hd24project currproj = *(currenthd24->getproject(i));
		string* projname1 = new string("");
		string* currpname = currproj.projectname();
		*projname1 += *currpname;
		
		cout << "======================================================================" << endl << "Project " << i << ": ";
		cout << *projname1 << endl;
		
		showsongs (&currproj);
		
		delete(currpname);
		delete(projname1);
	}
}

int main()
{
	hd24fs* fsysp=new hd24fs((const char*)NULL);
	hd24fs fsys=*fsysp;
	string* volname;		
	
	if (!fsys.isOpen()) 
	{
		cout << "Cannot open hd24 device." << endl;
		return 1;
	}
	
	int devcount = fsys.hd24devicecount();
	
	if (devcount > 1) 
	{
		cout << "Number of hd24 devices found: " << fsys.hd24devicecount() << endl;
	}
	
	for (int i = 0; i < devcount; i++) 
	{
		hd24fs* currhd24 = new hd24fs((const char*)NULL,fsys.mode(), i);
		
		if (devcount > 1) 
		{
			cout << "Showing info for device #" << i << endl;
		}
		
		volname = currhd24->volumename();
		string vname = "";
		vname += *volname;
		cout << "Volume name            : " << vname<<endl;
		delete volname;
		
		cout << "Number of projects     : " << currhd24->projectcount() << endl;
		showprojects(currhd24);
		delete currhd24;
	}
	
	return 0;
}
