# Dependencias

ao modificar as dependencias, rode:

- vcpkg install --triplet=x64-mingw-static

ao modificar o CMakeLists.txt, exclua a pasta "out".

# TO DO

- o programa atual só está funcionando para procurar variaveis com exact value, incremente o dropdown menu com a opção de: exact value, bigger then, smaller then, value between, inscreasced value, decresced value, changed value, uncanged value.
Também precisamos usar um EPISILON quando for buscar por variaveis com casas decimais (float e double), pois oque esta sendo exibido na tela pode ser uma aproximação.

- Também adicione essas opções de tipo de variavel: 1 byte, 2 bytes, 4 bytes, 8 bytes, All

- speed hack funcional