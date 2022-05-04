Servidor:

Registry - dados jogo:	tamanho tabuleiro (20x20 default), velocidade água (idk)
Produtor/Consumidor - Produtor: produz dados sobre o seguimento do jogo(informa á medida que a água passa um tudo)

#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#define MAX 256


int _tmain(int argc, LPTSTR argv[]) {
    HKEY chave;
    DWORD resposta, validar, versao = 1, opcao = 0, sizeval = 257 * sizeof(DWORD), sizeNome;
    TCHAR nomeChave[MAX], nomef[MAX] = _T("SOFTWARE\\Aula\\"), nome[MAX], val[MAX], docente[MAX];

#ifdef UNICODE 
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif
    //utilizar sempre

    _tprintf(_T("\Nome da chave: "));
    fflush(stdin);
    _fgetts(nomeChave, MAX, stdin);

    nomeChave[_tcslen(nomeChave) - 1] = '\0';     //tirar o /n

    _tcscat_s(nomef, MAX, nomeChave);
    

    if (RegCreateKeyEx(HKEY_CURRENT_USER, nomef,
        0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
        NULL, &chave, &resposta) != ERROR_SUCCESS)                         //criou com sucesso
    {
        DWORD error = GetLastError();
        _tprintf(_T("Errooa o criar a chave"));
    }
    else {
        if (resposta == REG_OPENED_EXISTING_KEY) {                             //criou nova key uma vez que ainda não existia
            _tprintf(_T("Chave aberta com sucesso  [%s]"), nomef);
        }
        else {
            _tprintf(_T("Chave criada com sucesso [%s]", nomef));
        }
    }
    RegCloseKey(chave);

    RegOpenKey(HKEY_CURRENT_USER, nomef, 0, KEY_ALL_ACCESS, &chave);

    
    _tprintf(_T("1 - criar atributo \n 2 - consultar os atribuos/valores \n 3 - eliminar atributo \n 4 - procurar atributo"));
    fflush(stdin);
    _fgetts(opcao, MAX, stdin);

    switch (opcao)
    {
    case 1:
        _tprintf(_T("Introduza o nome do atributo:"));
        fflush(stdin);
        _fgetts(nome, MAX, stdin);
        nome[_tcslen(nome) - 1] = '\0';     //tirar o /n
        _tprintf(_T("Introduza o valor do atributo:"));
        fflush(stdin);
        _fgetts(val, MAX, stdin);
        val[_tcslen(val) - 1] = '\0';     //tirar o /n
        RegSetValueEx(chave, nome,0, REG_SZ, (LPBYTE)val, (DWORD)(_tcslen(val)+1) * sizeof(TCHAR));
        break;

    case 2:
        _tprintf(_T("Consultar todos os atributos/valores"));
        DWORD i = 0;
        while (RegEnumValue(chave, i++, nome, &sizeNome, NULL, NULL, (LPBYTE)val, &sizeval) != ERROR_NO_MORE_ITEMS) {
            _tprintf(_T("\n[%d] - atributo [%s] - chave [%s]\n"), 1, nome, val);
            sizeNome = 257 * sizeof(TCHAR);
            sizeval = 257 * sizeof(TCHAR);
        }
        break;

    case 3:
        _tprintf(_T("Introduza o nome do atributo a apagar:"));
        fflush(stdin);
        _fgetts(nome, MAX, stdin);
        nome[_tcslen(val) - 1] = '\0';
        RegDeleteValue(chave, nome);
        break;
    
    case 4:
        _tprintf(_T("Introduza o nome do atributo a procurar:"));
        fflush(stdin);
        _fgetts(nome, MAX, stdin);
        nome[_tcslen(nome) - 1] = '\0';
        RegQueryValueEx(chave, nome, NULL,NULL,  (LPBYTE)val,&sizeval);
        _tprintf(_T("\natributo [%s] - valor [%s]\n"), nome, val);
        break;

    default:
        _tprintf(_T("Adeus"));
        break;
    }
    return 0;
}