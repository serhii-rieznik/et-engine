et-engine
=========

Cross-platform engine + framework, written on C++. One code for all platforms.

Supported platforms:
-------------
- iOS;
- OS X;
- Windows;
- Android (still porting);
- 
Features:
-------------
- wrappers for OpenGL and OpenGL ES objects;
- own GUI system (with multilingual support);
- own math library, similar to GLM and based on GLSL object naming convention;
- multithreading support;
- sound engine;
- gesture recognizers;
- collision test routines;
- support for PNG, JPG, DDS, PVR with asynchronous loading;
- locales support;
- FBX importer (using FBX SDK);
- basic primitive generation;
- basic scene graph with serialization support;
- LOD based terrain;

Platform-specific features:
-------------
- iOS specific features and wrappers: IAPs, MessageUI, CoreLocation, CoreMotion, UIImagePickerContorller, AVCaptureSession and other;
- touchpad support in OS X;
- 3rd-party wrappers: Chartboost;
- easy integration into Cocos2D-iphone or native iOS application.

Tools:
-------------
- texture atlas generator;
- FBX converter to native format;
- font generator.
