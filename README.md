# Hair Plugin

## About
This tool is a Houdini plugin that implements the 2025 SIGGRAPH paper, [Transforming Unstructured Hair Strands into Procedural Hair Grooms](https://dl.acm.org/doi/abs/10.1145/3731168). 

### Development Instructions
This plugin was made in **Houdini 21.0.631** and **Visual Studio 2022**, on Windows 11. Make sure you're using the same versions, or building the files might not work. 

In your environment variables, set the following system variables:
- H21_PATH: C:\Program Files\Side Effects Software\Houdini 21.0.631
- H21_VERSION: 21.0.631
- CUSTOM_DSO_PATH: C:\Users\\[YOUR USER]\Documents\Houdini\dso
   - This should be the location where you're storing all your `.dll` files. 
- HOUDINI_DSO_PATH: %CUSTOM_DSO_PATH%;&

**How to Build**

In the root directory, run:
- `cmake -G "Visual Studio 17 2022" -A x64 -S .\ -B .\Build`

This will create a Visual Studio solution. 

**Visual Studio Settings**

- After Visual Studio loads, right click on the project and select “Properties”.
- Change the path of the output directory to the path where your Houdini plugin dsos will be stored, i.e:
    - C:\Users\\[YOUR USER]\Documents\Houdini\dso
- In C++->General check the Additional Include Directory path properties to make sure it corresponds to where the Houdini toolkit include directory is located on your computer, i.e:
    - C:\Program Files\Side Effects Software\Houdini 21.0.631\toolkit\include

**Load into Houdini**

For the plugin to automatically load into Houdini, add this line into the houdini.env file in your Houdini install location, i.e. C:\Users\\[YOUR USER]\Documents\houdini21.0:
- `HOUDINI_DSO_PATH = C:\Users\[YOUR USER]\Documents\Houdini\dso`
