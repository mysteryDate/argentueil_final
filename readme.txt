This repo contains the final code for the installation at La Musée Regional d'Argenteuil by PLAYMIND. First installed on May 26th. Unfortunately prior commits are not in this repository as I tried to commit workspace files (from Visual Studio), and they were too large for git. 

Previous verions of this repo (mac compatible) can be found at: https://github.com/mysteryDate/argentueil_2nd_draft

This repo contains only source code and settings. All binaries (videos, images, etc.) are not included. These can be found in the repository linked to above for the smaller files (and to get an idea of the appropriate file structure).

To get the code up and running from scratch we will need:
	Microsoft visual studio
	Openframeworks (for Visual Studio): http://openframeworks.cc/download/
	ofxCv addon (for computer vision): https://github.com/kylemcdonald/ofxCv
	ofxFx (water ripple effect): https://github.com/patriciogonzalezvivo/ofxFX
	ofxTrueTypeFontUC (for accented characters): https://github.com/hironishihara/ofxTrueTypeFontUC
	ofxGstStandaloneVideo: https://github.com/outsidecontext/ofxGstStandaloneVideo

- Create a new project using the project generator, include the addons listed above, along with XMLSettings, OpenCV and Kinect.
- Move all nine source files in this repo to the src folder (and add them within Visual Studio)
- Recreate the folder structure of bin/data as in https://github.com/mysteryDate/argentueil_2nd_draft, copy over all files.
	-- Also create a bin/data/videos folder. Put the first video in as "p1.mov," second as "p2.mov," and screensaver as "screensaver.mov"
- Copy settings.xml into bin/data, overwrite any other copies

Now comes the hard part!

ofxGstStandaloneVideo is necessary to get HD video to run from OF on windows, but doesn't work out of the box. One needs to add dependances by hand, look at the example project in the addon and copy over extra dependances from three places:
	Click on C/C++ on the left and look into "Additional Include Directories", this is where you point the project to external source files. The example project should contain loads of references to "$(OSSBUILD_GSTREAMER_SDK_DIR)/" which is a system path to the gstreamer install. Then click on Linker and look into "Additional Library Directories", this is where you point to any compiled library files. Lastly, check Linker/Input/Additional Dependencies and grab the lib references from there.

ofxTrueTypeFontUC also has some problems with the VC++ compiler (the compiler that keeps on giving!). Follow what I did here to fix it: https://github.com/hironishihara/ofxTrueTypeFontUC/issues/8 (if it hasn't already been fixed).

After that, it should compile and run fine!
