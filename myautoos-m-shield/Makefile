# Ejecuta las pruebas del nucleo y de la simulacion. Variables utiles: CC=clang EXTRA=-Werror BUILD=/ruta
test:
	$(MAKE) -C core lint test
	$(MAKE) -C sim sil
coverage:
	$(MAKE) -C sim coverage
.PHONY: test coverage
