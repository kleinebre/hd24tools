#include <FL/forms.H>
#include <hd24driveimage.h>
hd24fs* testfs;
class hd24test
{
        friend class hd24fs;
public:
	static int runselftest(HD24UserInterface* ui,string* testdevname);
	static void test_prepareimage(string* testdevname);
	static unsigned char* getcopyofusagetable(hd24fs* testfs);
};

unsigned char* hd24test::getcopyofusagetable(hd24fs* testfs)
{
	return testfs->getcopyofusagetable();
}

void prepareimage_callback(void* y)
{
	cout << "." << endl;
	Fl::add_timeout(1.0,prepareimage_callback,y);	
	return;
}
int passcount;
int failcount;
#define MINSECTORS 1353964
#define MINSIZE (MINSECTORS*512)
//#define BITBIGGER (MINSECTORS+10000)
/* this gives for a bit over 5 minutes at 48k, 24 tracks: */
#define BITBIGGER (MINSECTORS+2073600)

int biggersize;

/* constant is for pretty demo image. */
void pass()
{
	passcount++;
	cout << "......................................................PASS" << endl;
}
void fail()
{
	failcount++;
	cout << "......................................................FAIL" << endl;
}
void fail(const char* reason)
{
	failcount++;
	cout << "......................................................FAIL" << endl;
	cout << "Reason: "<<reason << endl;
}
void hd24test::test_prepareimage(string* testdevname)
{
	if (testdevname!=NULL)
	{
		/* A device name was specified, indicating
		   that we want to perform tests on that given
		   device name or image rather than creating one
                   from scratch. This allows for quicker tests
                   to be run on an existing drive image (as making
                   an empty image is time intensive) or to run
                   tests on an actual physical hard drive. */
		if (*testdevname!="")
		{
			return;
		}
	}
	cout << "Test if creating too small drive images is prevented... " << endl;
	// counting starts at zero so last sector # of min. legal drive image is MINSECTORS-1
	// MINSECTORS-2 therefore should be too small.
	__uint32 lastsector=MINSECTORS-2;

	string* strtest=new string("testimage.h24");
	char message[2048];
	message[0]='\0';
   	Fl::add_timeout(1.0,prepareimage_callback,&message[0]);
	int result=hd24utils::newdriveimage(strtest,lastsector,&message[0],NULL);
	delete strtest;
	Fl::remove_timeout(&prepareimage_callback);
	cout << "Result is: " << message << endl;
	if (result!=-1) fail(); else pass();
	////////////////////////////////////////////////////
	cout << "-----------------------------------------------" << endl;
	cout << "Create minimum size empty drive image... please wait" << endl;
	lastsector=MINSECTORS-1;
	cout << "Drive should become " << MINSECTORS << " sectors in size." << endl;

	strtest=new string("smartimage.h24");
	hd24driveimage* image=new hd24driveimage();
	result=image->test(strtest); // internal tests, slow
        if (result!=0) return;

	image->initimage(strtest,lastsector);
	
	// Check expected drive image contents.
	cout << "Testing smart image FS mount..." << endl;
	hd24fs* testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false); // use newly created file as FS, no force.

	if (testfs->smartimage==NULL)
	{
		fail("Failed to mount smartimage.");
	} else {
		pass();
	}

	/* Before doing an actual quickformat, let's do a simple write test first. */
	unsigned char* testbuffer=(unsigned char*)malloc(1024);
	for (int i=0;i<1024;i++)
	{
		testbuffer[i]=(i%0xff);
	}
	testfs->writesectors(testfs->devhd24, 0,&testbuffer[0],1);
	testfs->writesectors(testfs->devhd24, 1,&testbuffer[1],1);
	testfs->writesectors(testfs->devhd24, 2,&testbuffer[2],1);
	free(testbuffer);

	testfs->quickformat(NULL);
	cout << "Verifying if commit after format went OK..." << endl;
	unsigned char* readbuffer=(unsigned char*)malloc(1024);
	testfs->readsectors(testfs->devhd24, lastsector,&readbuffer[0],1);
	hd24fs::dumpsector(readbuffer);
	free(readbuffer);
	cout << "That should say TADATSF somewhere." << endl;
	/* we calculate free space based on 1 channel because it is much
	   more sensitive than free space based on 24 channels */
	string* freespace=testfs->freespace(48000,1); 
	int lastsecerror=0;
	cout << "Free space on drive: " << *freespace << endl;

	hd24raw* rawfs=new hd24raw(testfs);
	lastsector=rawfs->getlastsectornum(&lastsecerror);
	unsigned char secbuffer[1024];
	rawfs->readsectors(lastsector,secbuffer,1);
	cout << "Dumping sector to screen for code coverage purposes." << endl;
	hd24fs::dumpsector((const unsigned char*)&secbuffer[0]);
	cout << "Is this a device file? ..." ;
	if (testfs->isdevicefile())
	{
		cout << "yes." << endl;
	} else {
		cout << "no." << endl;
	}
	int lastsec=(0x70);
	cout << "Testing if songentry2sector works as expected... " << endl;
	for (int i=0;i<(99*99);i++)
	{
		int x=testfs->songentry2sector(i);
		if (x!=(lastsec+7)) 
		{
			fail("Songs ought to be 7 sectors apart");
			break;
		}
		lastsec=x;	
	}
	pass();
	cout << "testing if 99x99 is an illegal song sector " << endl;
	if (testfs->songentry2sector(99*99)!=0) fail("99x99 ought to be an illegal song sector.");
	pass();
	delete testfs;
	delete rawfs;
	cout << "Expected last sector num is " << (MINSECTORS-1) << endl;
	cout << "Reported last sector num is " << lastsector << endl;
	if (lastsector!=(MINSECTORS-1)) { fail("Image size incorrect."); exit(1); }  else pass();

	//////////////////////////////////////////////////////
	cout << "-----------------------------------------------" << endl;
	cout << "Creating somewhat larger size empty drive image... please wait" << endl;
	lastsector=biggersize;
	cout << "(last sector of this drive image is supposed to be "<<lastsector<<")"<<endl;
		
	string* strtest2=new string("testimage2.h24");
	delete image;
	image=new hd24driveimage();
	image->initimage(strtest2,lastsector);

	// Check expected drive image contents.
	cout << "Testing smart image FS mount..." << endl;
	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest2,false); // use newly created file as FS, no force.
	testfs->quickformat(NULL);

	if (testfs->smartimage==NULL)
	{
		fail("Failed to mount smartimage.");
	} else {
		pass();
	}	
	cout << "Image create messages=" << message << endl;
	if (result==0) pass(); else fail();
	// Check expected drive image contents.
	hd24fs* testfs2=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest2,false);
	string* freespace2=testfs2->freespace(48000,1);
	cout << "Free space on drive: " << *freespace2 << endl;
	if (strcmp(freespace->c_str(),freespace2->c_str())==0)
	{
		fail("Images were supposed to have different sizes");
	} else {
		pass();
	}
	delete testfs2;
	delete freespace2;
	delete strtest2;
//*/
	delete freespace;
	delete strtest;
	return;
}

void test_useimage(string* testdevname)
{
	char failmsg[64];
	cout << "Try to use empty drive image... " << endl;
	//false=force?
	string* strtest=new string(testdevname->c_str());
	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;
	strtest=NULL;

	// Let's see if all is as expected.
	if (!(testfs->isOpen())) fail(); else pass();
	string* freespace=testfs->freespace(48000,24);
	cout << "Free space on drive: " << *freespace << endl;
	delete freespace;
	hd24raw* rawfs=new hd24raw(testfs);
	int lastsecerror=0;
	if (biggersize==0)
	{
		rawfs->quickformat(NULL);
		biggersize=rawfs->getlastsectornum(&lastsecerror);
	}
	/*
		These functions are only available through raw FS
	*/
	int songsondisk=rawfs->songsondisk();
	cout << "Songs on disk=" << songsondisk << endl;
	if (songsondisk!=0) fail(); else pass();

	__uint32 lastsector=rawfs->getlastsectornum(&lastsecerror);
	__uint32 expectedlastsector=biggersize;
	cout << "Last sectornum of drive image="<< lastsector << endl;
	
	
	if (lastsector!=expectedlastsector)
	{
		sprintf(failmsg,"Expected last sector was %ld but found %ld",expectedlastsector,lastsector);
		fail(failmsg);
	}
	else
	{
		pass();
	}

	string* devname=testfs->getdevicename(); 
	cout <<"Device name="<<*devname<<endl;
	if (*devname=="")
	{
		fail("Device name should always exist");
	} else pass();

	string* volname=testfs->volumename();
	cout <<"Volume name="<<*volname <<endl;
	if (*volname=="") fail("Volume name of freshly formatted drive must be nonempty"); else pass();

	// set volume name, commit, then test if change OK
	__uint32 projcount=testfs->projectcount();
	cout <<"Project count="<<projcount << endl;
	if (projcount!=1) fail("Newly formatted image must have exactly 1 project."); else pass();
	hd24project* currproj=NULL;
	if (projcount>0)
	{
		currproj=testfs->getproject(0); // must error
		cout << "Check protection against getting project 0" << endl;
		if (currproj!=NULL) fail(); else pass();

		delete currproj;
		currproj=testfs->getproject(1); // must not error
		cout << "Check zero songcount for supposedly empty project" << endl;
		__uint32 songcount=currproj->songcount();
		if (songcount!=0) fail(); else pass();

		cout << "Check getting nonexisting/invalid project numbers" << endl;
		int failed=0;
		for (int i=2; i<=100; i++)	
		{
			currproj=testfs->getproject(i); // must not error but must return null
			if (currproj!=NULL)
			{
				failed=1;
			}
		}
		if (failed!=0) fail(); else pass();
	}

	cout << "set volume name in drive image" << endl;	
	testfs->setvolumename("myvol"); // only in memory!!!
	// check if volume name has been set (in memory)
	cout << "reading back volume name to see if set was successful" << endl;

	volname=testfs->volumename();
	if (*volname!="myvol") fail(); else pass();

	cout << "set longer name in drive image" << endl;	
	testfs->setvolumename("mylongervolumename"); // only in memory!!!
	volname=testfs->volumename();
	if (*volname!="mylongervolumename") fail(); else pass();

	cout << "Test truncating too long volume name in drive image" << endl;
	testfs->setvolumename("myoverlylongvolumenamethemaximumissixtyfourcharactersbutwemakeitlongeranyway"); // only in memory!!!
	volname=testfs->volumename();
	if (strlen(volname->c_str())!=64) fail(); else pass();
	cout << "Try to clear volume name" <<endl;
	testfs->setvolumename("");
	volname=testfs->volumename();
	if (strlen(volname->c_str())!=0) fail(); else pass();
	cout << "Setting volume name " << endl;
	testfs->setvolumename("MyVolume");
	cout << "and commiting for later readback." << endl;	
	testfs->savedriveinfo(); // auto-commit
	cout << "Commit done." << endl;
	return;
}

void test_commit(string* testdevname)
{
	string* strtest=new string(testdevname->c_str());

	cout << "Delete testfs..." << endl;
	delete testfs;
	testfs=NULL;

	pass();

	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;
	cout << "Reading back volume name" << endl;
	string* volname=testfs->volumename();
	cout << "Read back " << *volname << endl;
	if (*volname!="MyVolume") fail(); else pass();

	// after commit, drive size must be preserved.
	hd24raw* rawfs=new hd24raw(testfs);
	int lastsecerror=0;
	__uint32 lastsector=rawfs->getlastsectornum(&lastsecerror);
	__uint32 expectedlastsector=biggersize;
 	cout << "Test if image has remained same size: "<< lastsector << endl;
	if (lastsector!=expectedlastsector) fail(); else pass();
	

	testfs->setvolumename("Drive Name");
	testfs->savedriveinfo(); // commit tested before-assume it succeeds.
}

void test_project(string* testdevname)
{
	string* strtest=new string(testdevname->c_str());

	cout << "Delete testfs..." << endl;
	delete testfs;
	testfs=NULL;
	
	pass();

	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;
	hd24raw* rawfs=new hd24raw(testfs);
	int lastsecerror=0;
	__uint32 lastsector=rawfs->getlastsectornum(&lastsecerror);
	__uint32 expectedlastsector=biggersize;
 	cout << "Test if image has still remained same size: "<< lastsector << endl;
	if (lastsector!=expectedlastsector) fail(); else pass();
	cout << "Checking current project count... (should be 1): ";
	int pcount=testfs->projectcount();	
	cout << pcount << endl;
	if (pcount==1) pass(); else fail("Project count of formatted drive should be 1");

	cout << "Try to get project id 0 (should not be possible)" << endl;
	hd24project* proj=testfs->getproject(0);
	if (proj==NULL) pass(); else
	{
		delete proj;
		proj=NULL;
		fail("Project IDs should be 1 based but are not!");
	}

	cout << "Try to get project id 1 (first and only project on drive" << endl;
	proj=testfs->getproject(1);
	if (proj==NULL) fail("Cannot get only project on drive"); else
	{
		delete proj;
		proj=NULL;
		pass();
	}
	cout << "Try to get any but the first project on the drive" << endl;
	cout << "(using legal project numbers and one too high number)" << endl;
	int projsgot=0;
	int i=0;
	for (i=2; i<=100; i++)
	{
		if (proj!=NULL)
		{
			delete proj;
			proj=NULL;
		}
		proj=testfs->getproject(i);
		if (proj!=NULL)
		{
			projsgot++;
			fail("Managed to get project number:");
			cout << i << endl;
			delete proj;
			proj=NULL;
		}
	}
	if (proj!=NULL)
	{
		delete proj;
		proj=NULL;
	}

	if (projsgot==0) pass();

	cout << "Try if only project on drive is protected from deleting" << endl;
	testfs->deleteproject(1);
	pcount=testfs->projectcount();	
	cout << pcount << endl;
	if (pcount==1) pass(); else fail("Only project on drive should not be deletable");

	cout << "Try to create a project..." << endl;
	hd24project* newproj=testfs->createproject("selftest project 1");
	if (newproj==NULL)
	{
		fail("Creating project failed");
	} else {
		pass();
	}
	delete newproj; // we won't be using this object anymore.

	cout << "Make sure project creation updates project count..." << endl;
	pcount=testfs->projectcount();	
	if (pcount==2) pass(); else fail("Project count should have been 2 at this stage.");
	cout << "Trying to delete newly created project..." << endl;
	testfs->deleteproject(2);
	pcount=testfs->projectcount();	
	if (pcount==1) pass(); else fail("Deletion successful");

	cout << "Trying to create 98 new projects (should be possible)..." << endl;
	char projname[64];
	int failedcreate=0;	
	for (i=0;i<98;i++)
	{
		cout << "project=" << i << endl;
		sprintf(projname,"selftest project %d",i+2);
		hd24project* newproj=testfs->createproject(projname);
		pcount=testfs->projectcount();	
		if (pcount!=(i+2))
		{
			sprintf(projname,"could not create project %d",i);
			fail(projname);
			failedcreate=1;
			break;
		};
		delete newproj;
	}
	if (failedcreate==0)
	{
		pass();
	}
	cout << "Checking if project count is 99 as expected " << endl;
	pcount=testfs->projectcount();	
	if (pcount==99) pass(); else fail("Nope.");

	cout << "Trying if creating more projects than allowed is prevented" << endl;
	newproj=NULL;
	newproj=testfs->createproject(projname);
	if (newproj!=NULL)
	{
		fail("Project object was returned!");
	} else {
		pass();
	}
	
	pcount=testfs->projectcount();	
	cout << "Verifying project count to still be 99" <<endl;
	if (pcount==99) pass(); else fail("Project count unduely updated.");

	/* Code coverage: make sure one project is nonempty to 
           cover deleting of non-empty project. */
	proj=testfs->getproject(70);
	for (i=1;i<10;i++)
	{
		hd24song* currsong=proj->createsong("A Huge Hit",24,48000);
		delete currsong;
	}
	unsigned char* usagetable=hd24test::getcopyofusagetable(testfs);
	testfs->dumpclusterusage(usagetable); // should be all zeros?
	testfs->dumpclusterusage2(usagetable);
	free(usagetable);

	cout << "Deleting last 50 projects" << endl;
	/* this should leave 49 projects in place. */
	for (i=1;i<60;i++)
	{
		testfs->deleteproject(50);
	}
	pcount=testfs->projectcount();	
	if (pcount==49) pass(); else fail("Incorrect project count.");
	cout << "Creating 50th project again" << i << endl;
	sprintf(projname,"selftest project %d",i+2);
	if (newproj!=NULL) delete newproj;

	newproj=testfs->createproject(projname);
	pcount=testfs->projectcount();
	cout << "Expecting project count to be 50 now..." << endl;	
	if (pcount!=50) fail("Incorrect project count"); else pass();

	cout << "Checking project sector of first project (must be 0x14 or 20)" << endl;
	__uint32 projsec= rawfs->getprojectsectornum(1);
	cout << "Proj sector=" << projsec << endl;
	if (projsec!=0x14) fail("Incorrect sector"); else pass();
	
	cout << "Deleting first project (to empty a slot)" << endl;
	pcount=testfs->projectcount();	
	cout << "project count BEFORE=" <<pcount <<endl;

	testfs->deleteproject(1);
	int pcount2=testfs->projectcount();
	cout << "Project count AFTER=" <<pcount2 << endl;	
	if (pcount2!=(pcount-1)) fail("Expected project count to be 49."); else pass();
	projsec= rawfs->getprojectsectornum(1);
	cout << "Second project should originally have been on 0x15,"<<endl;
	cout << "expecting first project to start on that sector now..." <<endl;
	cout << "(in reality it is on " << projsec << ")" << endl;
	if (projsec!=0x15) fail("Incorrect sector"); else pass();
	/* now create a new project-- this project must be on sector 0x14
	again */
	cout << "Creating new project - it should overwrite the first project block" << endl;
	newproj=testfs->createproject(projname);
	pcount=testfs->projectcount();	
	cout << "Expecting project count of 50 again, in reality it is " << pcount << endl;
	if (pcount!=50) fail("Incorrect project count"); else pass();
	__sint32 projid=newproj->projectid();
	cout << "Project id=" << projid << endl;
	if (projid!=50) {
		fail("Unexpected project id");
	}
	projsec=rawfs->getprojectsectornum(projid);
	cout << "Expecting project sector of project " << projid << " to be 0x14," ;
	cout << " in reality it is " << projsec << endl;

	if (projsec!=0x14) fail("Incorrect sector"); else pass();

	newproj->projectname("Changed Name");
	string* projcname=newproj->projectname();
	if (*projcname!="Changed Name") fail("Project name change unsuccessful"); else pass();
	delete projcname;
	newproj->save();
	delete rawfs;		
}


void test_proj2(string* testdevname)
{
	int pcount;
	string* strtest=new string(testdevname->c_str());

	cout << "Delete testfs..." << endl;
	delete testfs;
	testfs=NULL;

	pass();

	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;
	cout << "Checking current project count... (should be 50): ";

	pcount=testfs->projectcount();	
	cout << "Expecting project count of 50 again, in reality it is " << pcount << endl;
	if (pcount!=50) fail("Incorrect project count"); else pass();
	cout << "Getting existing project no. 50..." << endl;
	hd24project* currproj=testfs->getproject(50);
	if (currproj==NULL) { fail("Cannot get project"); } else {pass();}
	cout << "Getting project name: ";
	string* pname=currproj->projectname();
	cout << *pname << endl;
	if (*pname!="Changed Name") fail("Project name change unsuccessful"); else pass();
	
//	delete rawfs;		
	
}

void test_song(string* testdevname)
{
	string* strtest=new string(testdevname->c_str());

	cout << "Delete testfs..." << endl;
	delete testfs;
	testfs=NULL;

	pass();

	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;

	// Check on-disk songcount
	hd24raw* rawfs=new hd24raw(testfs);
	int songcount=rawfs->songsondisk();
	cout << "Checking if song count is still zero as it should be "<<endl;
	if (songcount!=0)
	{
		fail("Song count of newly formatted drive should be 0.");
	} else {
		pass();
	}

	delete rawfs;		
	
}

void test_demoimage(string* testdevname)
{
	string* strtest=new string(testdevname->c_str());

	cout << "Delete testfs..." << endl;
	delete testfs;
	testfs=NULL;

	pass();

	testfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,strtest,false);
	delete strtest;
	
	testfs->setvolumename("Sizeable Hard Drive"); // only in memory!!!
	testfs->commit();
	
	string* volname=testfs->volumename();
	if (*volname!="Sizeable Hard Drive") fail(); else pass();
	delete volname;

	cout << "Checking FS version..." << endl;
	string* version=testfs->version();
	if (*version!="1.10") fail("Unexpected FS version (expectation was 1.10)"); else pass();
	delete version;

	cout << "Checking max. project count (99 expected)..." << endl;
	__uint32 maxprojcount=testfs->maxprojects();
	if (maxprojcount!=99) fail("Unexpected max project count"); else pass();

	cout << "Checking block size..." << endl;
	if (testfs->getblocksizeinsectors() != 0x480) fail("Unexpected block size"); else pass();

	// delete all but last project
	while (testfs->projectcount()>1)
	{
		// leave only last project	
		testfs->deleteproject(2);
	}

	// now up the total project count to 42	
	char projname[64];
	int i=1;
	hd24project* newproj=NULL;
	while (testfs->projectcount()!=42)
	{
		i++;
		sprintf(projname,"selftest project %d",i+2);
		int prevprojcount=testfs->projectcount();
		newproj=testfs->createproject(projname);
		int newprojcount=testfs->projectcount();
		if (newprojcount==prevprojcount)
		{
			fail("Failed to create project.");
			break;
		}
		if (newproj!=NULL) 
		{
			delete newproj;
			newproj=NULL;
		}
			
	}
	if (newproj!=NULL)
	{
		delete newproj; // we won't be using this object anymore.
		newproj=NULL;
	}

	// rename project now.
	hd24project* currproj;
	currproj=testfs->getproject(15); // must error
	testfs->lastprojectid(15);
	currproj->projectname("Client X");
	currproj->save();
	volname=testfs->volumename();
	cout <<"Volume name="<<*volname <<endl;
	testfs->commit();
	delete currproj;
	currproj=NULL;
	currproj=testfs->getproject(15);
	cout << "Test if project name change with commit works..." << endl;
	cout << "Projname is supposed to be 'Client X', and is: ";
	string* sprojname=currproj->projectname();
	cout << *sprojname << endl;
	if (*sprojname!="Client X") fail("project name change failed."); else pass();
	delete sprojname;
	cout << "Testing if song count is zero..." << endl;
	if (currproj->songcount()==0) pass(); else fail("Should have been 0");

	// Now it's time to create some songs.
	for (i=0;i<12;i++)
	{
		int samrate=48000;
		if (i==9) {
			samrate=44100;
		}
		if (i==10) {
			samrate=88200;
		}
		if (i==11) {
			samrate=96000;
		}
		if (samrate>=88200)
		{
			cout << "Making track count of high samplerate songs"
				" is limited..." << samrate << endl;
		}
		hd24song* currsong=currproj->createsong("A Huge Hit",24,samrate);

		// for now intended to verify code coverage only
		if (samrate>=88200)
		{
			
 			if (currsong!=NULL)
			{
				fail("Managed to create high samplerate song with >12 tracks");
			} else {
				pass();
				currsong=currproj->createsong("A Huge Hit",6,samrate);
			}
		}
		if ((samrate<=48000) and (currsong==NULL))
		{
			fail("Did not manage to create low samplerate song");
		}
		if (currsong!=NULL)
		{
			currsong->save();
			string* locpointfile=new string("bin/loc/testloc.loc");
			currsong->savelocpoints(locpointfile); 
			delete locpointfile;
			string* newlocname=new string("newloc");
			currsong->setlocatename(-1,*newlocname);
			currsong->setlocatename(0,*newlocname);
			currsong->setlocatename(3,*newlocname);
			string* newlocname2=new string("toolongnewlocname");
			currsong->setlocatename(4,*newlocname2);
			// TODO: Verify if setting locpoints worked.
			locpointfile=new string("bin/loc/nonexisting.loc");
			currsong->loadlocpoints(locpointfile);
			delete locpointfile;
			locpointfile=new string("bin/loc/invalid.loc");
			currsong->loadlocpoints(locpointfile);
			delete locpointfile;
			// TODO: compare if locnames are restored
			// when re-loading file!	
			delete newlocname;
			delete newlocname2;
		}
		delete currsong;
	}
	currproj->lastsongid(8);
	currproj->save();
	testfs->commit();
	hd24song* currsong=currproj->getsong(currproj->lastsongid());
	cout << "Renaming song..." << endl;
	currsong->songname("A Huge Hit (final take)");
	currsong->save();
	testfs->commit();
	delete currsong;
	currsong=NULL;
	currsong=currproj->getsong(currproj->lastsongid());
	cout << "Checking if song ID is equal to last set..." << endl;
	if (currsong->songid()==8) pass(); else fail("Incorrect songid.");
	string* freespacebefore=testfs->freespace(48000,24);
	cout << "Lengthening song..." << endl;
	__uint32 oldlen=currsong->songlength_in_wamples();
	__uint32 desiredlen=10416000; //00:03:37.00
	if (oldlen==desiredlen) fail("Song was already the desired length.");
	__uint32 newlen=currsong->songlength_in_wamples(desiredlen);
	if (newlen!=desiredlen)
	{
		fail("Song lengthening failed.");  
		cout << "Length in samples is " << newlen 
		     << "instead of " << desiredlen << endl;
	} else pass();
	
	currsong->save();
	testfs->commit();
	unsigned char* usagetable=hd24test::getcopyofusagetable(testfs);
	testfs->dumpclusterusage(usagetable); // should be all zeros?
	testfs->dumpclusterusage2(usagetable);
	free(usagetable);
	cout << "Free space on drive before lengthening: " << *freespacebefore << endl;
	string* freespace=testfs->freespace(48000,24);
	cout << "Free space on drive after lengthening: " << *freespace << endl;
	delete freespace;
	if (strcmp(freespace->c_str(),freespacebefore->c_str())==0)
	{
		fail("Free space was supposed to have changed.");
	} else {
		pass();
	}
	currsong->setwriteprotected(true);
	currsong->save();
	testfs->commit();
	// make the start of the image look a bit more interesting
	// for screenshot purposes

	delete currproj;
	currproj=NULL;
	currproj=testfs->getproject(1);
	currproj->projectname("Client A");
	currsong=currproj->createsong("Song One",24,44100);
	currsong->songlength_in_wamples(((60+15)*44100));
	currsong->save();
	delete currsong;
	currsong=currproj->createsong("Song Two",24,44100);
	currsong->songlength_in_wamples((35*44100));
	currsong->save();
	delete currsong;
	currsong=currproj->createsong("Song Three",24,44100);
	currsong->songlength_in_wamples((44*44100));
	currsong->save();
	delete currsong;
	currproj->save();

	delete currproj;
	currproj=NULL;
	currproj=testfs->getproject(2);
	currproj->projectname("Client B");
	currsong=currproj->createsong("Testing",24,44100);
	currsong->songlength_in_wamples((62*44100+4412)); // 00:01:02.03
	currsong->save();
	delete currsong;
	currsong=currproj->createsong("Is this on",24,44100);
	currsong->songlength_in_wamples((10*44100));
	currsong->save();
	delete currsong;
	currproj->save();

	delete currproj;
	currproj=NULL;
	currproj=testfs->getproject(3);
	currproj->projectname("Anonymous client");
	currsong=currproj->createsong("Thunderstruck",24,44100);
	currsong->setwriteprotected(true);
	currsong->songlength_in_wamples((360+34)*44100); // 00:01:02.03
	currsong->save();
	delete currsong;
	currproj->save();

	delete currproj;
	currproj=NULL;
	currproj=testfs->getproject(4);
	currproj->projectname("Four brunettes");
	currsong=currproj->createsong("I say hey",24,44100);
	currsong->save();
	delete currsong;
	currsong=currproj->createsong("Hey ay ay",24,44100);
	currsong->save();
	delete currsong;
	currsong=currproj->createsong("What is happening",24,44100);
	currsong->save();
	delete currsong;
	currproj->save();
	delete currproj;

	currproj=NULL;
	currproj=testfs->getproject(5);
	currproj->projectname("The famous five");	

	currproj->save();
	delete currproj;

}

int hd24test::runselftest(HD24UserInterface* ui,string* testdevname)
{
	cout << "=======TESTMODE ACTIVATED=======" << endl;
	passcount=0;
	failcount=0;
	int wasnullname=0;
	int emptyname=0;
	biggersize=0;
	if (testdevname==NULL)
	{
		emptyname=1; 
		biggersize=BITBIGGER;
	}
	else 
	{
		if (*testdevname=="")
		{ 
			emptyname=1;
			biggersize=BITBIGGER;
		}
	}
	
	test_prepareimage(testdevname);
	if (emptyname==1)
	{
		testdevname=new string("testimage2.h24");
		wasnullname=1;
	}

	test_useimage(testdevname);
	test_commit(testdevname);
	test_project(testdevname);
	test_proj2(testdevname);

	test_song(testdevname);
	test_demoimage(testdevname); // create an interesting looking demo drive image

	// TODO:
	// - write audio files
	// - write audio to songs
	// - transfer audio (back) to file, compare with orig. file
	// - silence tracks
	// etc
	
	cout << "Record audio to songs" << endl;
	
	// Proceed to test gui.

	Fl_Window* window=ui->make_window(testfs);
	window->end();
	window->show();
	fl_message("%s","Proceeding to test GUI.");
	cout << "====================== TEST COMPLETE ===================" << endl;
	cout << "PASS: " << passcount << endl;
	cout << "FAIL: " << failcount << endl;
	cout << "========================================================" << endl;
	if (wasnullname==1)
	{
		delete testdevname;
		testdevname=NULL;
	}
	return 0;
}
