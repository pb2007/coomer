#!/bin/sh
valgrind --tool=memcheck --leak-check=full ./coomer
