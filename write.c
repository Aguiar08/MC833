char email[50], nome[50], sobrenome[50], residencia[50], formacao[50], habilidades[100];
    int ano_formatura;
    printf("Digite o seu email: ");
    if ((numbytes = recv(new_fd, email, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

	email[numbytes] = '\0';
    printf("%s", email);
    return 0;
    printf("Digite o seu nome: ");
    scanf("%s", nome);
    printf("Digite o seu sobrenome: ");
    scanf("%s", sobrenome);
    printf("Digite a sua residencia: ");
    scanf("%s", residencia);
    printf("Digite a sua formacao academica: ");
    scanf("%s", formacao);
    printf("Digite o ano de sua formatura: ");
    scanf("%d", &ano_formatura);
    printf("Digite suas habilidades: ");
    fgets(habilidades, 100, stdin);
    FILE *file = fopen(email, "w");
    if (file == NULL) {
        printf("Erro ao criar arquivo!\n");
        return 1;
    }
    fprintf(file, "Email: %s\n", email);
    fprintf(file, "Nome: %s Sobrenome: %s\n", nome, sobrenome);
    fprintf(file, "Residencia: %s\n", residencia);
    fprintf(file, "Formacao Academica: %s\n", formacao);
    fprintf(file, "Ano de Formatura: %d\n", ano_formatura);
    fprintf(file, "Habilidades: %s\n", habilidades);
    fclose(file);
    printf("server: Perfil criado e salvo em perfil.txt com sucesso!\n");
    return "Perfil criado e salvo em perfil.txt com sucesso!";