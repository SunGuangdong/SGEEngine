# Zips the game files for web builds. Suitable for itch.io

import zipfile

lista_files = ["CharacterGame.data","CharacterGame.js","CharacterGame.wasm","index.html"]
with zipfile.ZipFile('out.zip', 'w') as zipMe:        
    for file in lista_files:
        zipMe.write(file, compress_type=zipfile.ZIP_DEFLATED)