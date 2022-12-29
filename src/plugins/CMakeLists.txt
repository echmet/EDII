cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

option(EDII_PLUGIN_ENABLE_ASC "Enable plugin to load ASC files" ON)
option(EDII_PLUGIN_ENABLE_CSV "Enable plugin to load CSV files" ON)
option(EDII_PLUGIN_ENABLE_HPCS "Enable plugin to load HP/Agilent ChemStation files" ON)
option(EDII_PLUGIN_ENABLE_NETCDF "Enable plugin to load NetCDF files" ON)
option(EDII_PLUGIN_ENABLE_EZCHROM "Enable plugin to load EZChrom/32Karat files" ON)

if (EDII_PLUGIN_ENABLE_ASC)
    add_subdirectory(ASCSupport)
endif ()

if (EDII_PLUGIN_ENABLE_CSV)
    add_subdirectory(CSVSupport)
endif ()

if (EDII_PLUGIN_ENABLE_HPCS)
    add_subdirectory(HPCSSupport)
endif ()

if (EDII_PLUGIN_ENABLE_NETCDF)
    add_subdirectory(NetCDFSupport)
endif ()

if (EDII_PLUGIN_ENABLE_EZCHROM)
    add_subdirectory(EZChromSupport)
endif ()
