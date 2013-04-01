

import os

flags = [
'-Wall',
'-Wextra',

'-std=c99',
'-fno-exceptions',
'-fno-rtti',
'-DUSE_CLANG_COMPLETER',
'-DUSE_STDPERIPH_DRIVER',
'-DARM_MATH_CM4'

'-x', 'c',
'-I', '.',
'-I', './Libraries/CMSIS/Device/ST/STM32F30x/Include',
'-I', './Libraries/STM32_USB-FS-Device_Driver/inc',
'-I', './Libraries/CMSIS/Include',
'-I', './Utilities/STM32F3_Discovery',
'-I', './Libraries/STM32F30x_StdPeriph_Driver/inc',
]

def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )


def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
  if not working_directory:
    return flags
  new_flags = []
  make_next_absolute = False
  path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith( '/' ):
        new_flag = os.path.join( working_directory, flag )

    for path_flag in path_flags:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith( path_flag ):
        path = flag[ len( path_flag ): ]
        new_flag = path_flag + os.path.join( working_directory, path )
        break

    if new_flag:
      new_flags.append( new_flag )
  return new_flags

def FlagsForFile(filename):
    return {
        'flags': MakeRelativePathsInFlagsAbsolute(flags, DirectoryOfThisScript()),
        'do_cache': True
    }

