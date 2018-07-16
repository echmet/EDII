ECHMET Data Import Infrastructure
===

Introduction
---
ECHMET Data Import Infrastructure (EDII) is a pluginable framework for import of chromatographic and electrophoretic experimental data from various sources. It is designed to run as a separate background service. Communication with other software is accomplished via IPC interfaces. Pluginable architecture allows for easy addition of support for new data formats. Individual plugins may use Qt5 toolkit to provide GUI interface. 

Supported data formats
---
EDII currently provides plugins to import following data formats
- Agilent (HP) ChemStation / OpenLab
- ASC text file (Beckman-Coulter 32Karat, EZChrom)
- Comma separated values (CSV) 
- NetCDF (currently tested only with 32Karat output)

Suported IPC interfaces
---
- Local TCP socket (everywhere)
- [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) (Linux)

Dependencies
---
EDII depends on the following toolkits and libraries in order to be built and run
- [Qt 5 toolkit](https://www.qt.io/)
- [CMake](https://cmake.org/)

Additionally, plugins that are shipped along with the library have the following dependencies
- [libHPCS](https://github.com/echmet/libHPCS) - HPCSSupport plugin
- [NetCDF](https://www.unidata.ucar.edu/software/netcdf/) - NetCDFSupport plugin
- [ICU](http://site.icu-project.org/home) - ASCSupport plugin, required only on UNIX systems

Building
---
EDII relies on the CMake build system to generate appropriate make or project files.

<a name="Linux_UNIX"></a>
#### Linux/UNIX
`cd` to the directory with EDII sources and run the following sequence of commands

	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make

If any of the dependencies is not installed in a globally used directory, you may use the following parameters for CMake to specify their locations manually.

- `-DICU_ROOT=` Path to ICU library installation
- `-DLIBHPCS_DIR=` Path to libHPCS library installation
- `-DLIBNETCDF_DIR=` Path to NetCDF library installation

#### Windows
Use `CMake-gui` tool to set up the project and generate project files for your compiler of choice. Keep in  mind that since Qt5 exports C++ objects, EDII must be built with the same compiler that was used to build your Qt5 toolkit. Refer to the [Linux/UNIX](#Linux_UNIX) section of this README for details how to set up paths for the required dependencies.

Licensing
---
ECHMET Data Import Infrastructure is distributed under the terms of [The Lesser GNU General Public License v3](https://www.gnu.org/licenses/lgpl.html) (GNU LGPLv3).

Author
---
Michal Malý

Group of Electromigration and Chromatographic Methods (http://echmet.natur.cuni.cz)

Department of Physical and Macromolecular Chemistry
Faculty of Science, Charles University in Prague, Czech Republic
