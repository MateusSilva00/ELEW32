/*
 * O startup da ST chama __libc_init_array(), da newlib, antes de main.
 * Com -nostartfiles, fornecemos o hook _init que normalmente viria de crti.o.
 * Nao ha inicializacao adicional da biblioteca C nesta aplicacao.
 */
void _init(void)
{
}
