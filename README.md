Ensure
======

Validator application for EFL programs.


Setup
-----

For now you need to tweak your ecore_evas:
	/static Ecore_Evas *ecore_evases/s/static//;
Then make and install ecore again.

Building Ensure
---------------

Just a `Makefile' at the moment, and no install.
    make

Running Ensure
--------------

    ./ensure /path/to/your/app --your --app -args

Status
------

Doesn't actually do anything useful yet.

Future Tools
------------

   * Image objecthas valid image file
   * Invalid colour
   	- A > max(r,g,b)
	- A == 0 (invisible)
   * Object clipped to a smart's clip, but not a member
   * Object not clipped to it's smart parrent (subtly different)
   * Object of size 0
   * Object outside it's clip
   * Object outside of smart object
   * Smart clip's size != smart object's size
   * Scaled images
   * Text with unusual paramaters

