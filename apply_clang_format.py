# This file applies the .clang-format to the code, exculding 3rd parties.
# It is assumed that there is already installed and acessible (for all users) LLVM 10 or newer.
# This script is tested with Python 3.

from glob import glob
from pathlib import Path
import os, sys, subprocess

# Check if the .clang-format file exists. If not abort the formatting,
# because the tool will apply some defaults, which do not format the code the way we want.
if not os.path.isfile('.clang-format'):
	print('.clang format file cannot be found! Aborting!')
	os.system("PAUSE")
	sys.exit()

# Obtain all source files that we want to format.
filesToFormat = []
directoriesToFormat = ['libs', 'samples', 'sge_editor', 'sge_player']
extension = {'.h', '.hpp', '.c', '.cpp'}

for dir in directoriesToFormat:
	for path in Path(dir).glob(r'**/*'):
		if path.suffix in extension:
			filesToFormat.append(str(path))

# Apply the formatting.
for file in filesToFormat:
	print(file)
	output = subprocess.run(["clang-format", "-i", "-style=file",  file])

	# if clang-format returns something assume that it is an error!
	if output.returncode != 0:
		print("clang-format failed with:\n" + str(output))
