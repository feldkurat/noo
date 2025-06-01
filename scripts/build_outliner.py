#!/usr/bin/python3

import typing
import platform
import os
import shutil
import glob
from pathlib import Path
import multiprocessing
import build_utils

EXIT_OK = 0
EXIT_ERROR = 1

# Check if Qt is specified
if not 'QT_HOME' in os.environ:
    print('Qt location must be set in QT_HOME environment variable.')
    exit(1)

# Prepare build directory
build_dir = Path('build')
if build_dir.exists():
    shutil.rmtree(build_dir)
os.mkdir(build_dir)

app_source = Path('../client').resolve()
version_suffix = build_utils.get_version(app_source / 'config.h', 'APP_VERSION_SUFFIX') 
version_minor = build_utils.get_version(app_source / 'config.h', 'APP_VERSION_MINOR')
version_major = build_utils.get_version(app_source / 'config.h', 'APP_VERSION_MAJOR')

if version_major is None or version_minor is None or version_suffix is None:
    print('App version is not found, exiting.')
    exit(EXIT_OK)
    
app_version = f'{version_major}.{version_minor}.{version_suffix}'
print (f'Found Noo version: {app_version}')

# Go to build directory
os.chdir(build_dir)

if platform.system() == 'Linux':
    print('Linux detected')
    print('Configure...')

    qt_home = Path(os.environ['QT_HOME']).resolve()
    # cmake_params = {
    #         'Qt5Core_DIR':           qt_home / 'lib/cmake/Qt5Core',
    #         'Qt5Widgets_DIR':        qt_home / 'lib/cmake/Qt5Widgets',
    #         'Qt5DBus_DIR':           qt_home / 'lib/cmake/Qt5DBus',
    #         'Qt5Gui_DIR':            qt_home / 'lib/cmake/Qt5Gui',
    #         'Qt5LinguistTools_DIR':  qt_home / 'lib/cmake/Qt5LinguistTools',
    #         'Qt5PrintSupport_DIR':   qt_home / 'lib/cmake/Qt5PrintSupport',
    #         'Qt5OpenGL_DIR':         qt_home / 'lib/cmake/Qt5OpenGL',
    #         'Qt5Network_DIR':        qt_home / 'lib/cmake/Qt5Network'
    #         }

    # cmake_cmd = 'cmake '
    # for k,v in cmake_params.items():
    #     cmake_cmd = f'{cmake_cmd} -D{k}={v} '
    # cmake_cmd = f'{cmake_cmd} ../../client'
    # print(f'cmake command line: {cmake_cmd}')

    # retcode = os.system(f'{cmake_cmd} ../../client')
    retcode = os.system(f'{qt_home}/bin/qmake ../../client')
    if retcode != 0:
        print(f'qmake call failed with code {retcode}')
        exit(retcode)

    print('Build...')
    retcode = os.system('make -j4')
    if retcode != 0:
        print(f'make call failed with code {retcode}')
        exit(retcode)

    # Build appimage
    print('Assembling app...')
    os.chdir('..')

    # Remove possible old image
    if os.path.exists('appimage_dir'):
        shutil.rmtree('appimage_dir')
    
    # Expand image template
    retcode = os.system('tar -xvzf appimage_dir.tar.gz')
    if retcode != 0:
        print(f'Failed to expand template directory, code {retcode}')
        exit(retcode)

    # Copy binary file
    shutil.copy('build/noo', 'appimage_dir/usr/bin')

    deploy_options = [
        '-always-overwrite', 
        '-verbose=2', 
        '-appimage', 
        '-qmake=' + os.environ['QT_HOME'] + '/bin/qmake', 
        '-unsupported-allow-new-glibc', 
        #'-no-translations', 
        '-extra-plugins=iconengines,platformthemes/libqgtk3.so'
    ]

    desktop_path = 'appimage_dir/usr/share/applications/noo.desktop'
    cmd_deploy = f'./linuxdeployqt {desktop_path} {" ".join(deploy_options)}'
    retcode = os.system(cmd_deploy)
    if retcode != 0:
        print(f'linuxdeployqt failed with code {retcode}')
        print(cmd_deploy)
        exit(retcode)

    releases_dir = Path('releases')
    if not releases_dir.exists():
        os.mkdir(releases_dir)
    for f in os.listdir():
        if f.endswith('x86_64.AppImage') and f.startswith('Noo'):
            shutil.move(f, releases_dir / f'noo-{app_version}-x86_64.AppImage')
            
elif platform.system() == 'Windows':
    print('Windows detected')
    print('Configure...')

    qt_home = Path(os.environ['QT_HOME']).resolve()
    if len(str(qt_home)) == 0:
        raise RuntimeError('Qt not found')
    
    retcode = os.system(f'{qt_home}/bin/qmake ../../client')
    if retcode != 0:
        print(f'qmake call failed with code {retcode}')
        exit(retcode)

    # print('Build...')
    # retcode = os.system('make -j4')
    # if retcode != 0:
    #    print(f'make call failed with code {retcode}')
    #    exit(retcode)

    # Build appimage
    # print('Assembling app...')
    # os.chdir('..')

    # Remove possible old image
    # if os.path.exists('appimage_dir'):
    #    shutil.rmtree('appimage_dir')
    
    # Expand image template
    #retcode = os.system('tar -xvzf appimage_dir.tar.gz')
    #if retcode != 0:
    #    print(f'Failed to expand template directory, code {retcode}')
    #    exit(retcode)

    # Copy binary file
    # shutil.copy('build/noo', 'appimage_dir/usr/bin')

    # deploy_options = [
    #     '-always-overwrite', 
    #     '-verbose=2', 
    #     '-appimage', 
    #     '-qmake=' + os.environ['QT_HOME'] + '/bin/qmake', 
    #     '-unsupported-allow-new-glibc', 
    #     #'-no-translations', 
    #     '-extra-plugins=iconengines,platformthemes/libqgtk3.so'
    # ]

    # desktop_path = 'appimage_dir/usr/share/applications/noo.desktop'
    # cmd_deploy = f'./linuxdeployqt {desktop_path} {" ".join(deploy_options)}'
    # retcode = os.system(cmd_deploy)
    # if retcode != 0:
    #     print(f'linuxdeployqt failed with code {retcode}')
    #     print(cmd_deploy)
    #     exit(retcode)

    # releases_dir = Path('releases')
    # if not releases_dir.exists():
    #     os.mkdir(releases_dir)
    # for f in os.listdir():
    #     if f.endswith('x86_64.AppImage') and f.startswith('Noo'):
    #         shutil.move(f, releases_dir / f'noo-{app_version}-x86_64.AppImage')
    
