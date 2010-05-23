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

Currently checks for:
   * Object of size 0
   * Invalid colour
   	- A > max(r,g,b)
	- A == 0 (invisible)
   * Text with trailing spaces
   * Object of negative size
   * Object off screen
   * Empty text


Future Tools
------------

   * Image object has valid image file
      * Object clipped to a smart's clip, but not a member
   * Object not clipped to it's smart parent (subtly different)
   * Objects of size 1x1: Probably default not resized
   * Objects of size 32x32: Probably a smart object not resized
   * Object outside it's clip
   * Object outside of smart object
   * Smart clip's size != smart object's size
   * Objects clipped to a smarts clip are also smart members
   * Scaled images
   * Text with unusual paramaters
   * Font 5 pts or less
   * Font 100 pts of more

