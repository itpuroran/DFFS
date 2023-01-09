Это версия программы SSAGES (https://github.com/SSAGESproject/SSAGES), адаптированная для расчетов частоты нуклеации при кристаллизации и кавитации жидкости методом Direct Forward Flux Sampling (DFFS).

### Отличия от базовой версии

* Добавлена возможность перезапуска расчета (эта функция заявлена в документации SSAGES, но не релизованна). Во входном файле формата json необходимо указать дополнительный параметр NLastSuccessful - число сохраненных конфигураций на последней достигнутой поверхности.
* Параллельный расчет (на один walker может приходиться более 1го процесора).
* Данные о пересечении промежуточных поерхностей записываются в отдельные файлы в папке outputs.
* Добавлены новые параметры порядка.
* В файлы fail-... для экономии места на диске пишется только шапка.
* Параметр writeresults в файле params.txt указывает, нужно ли печатать значения параметра порядка.
* Параметр writestruct в файле params.txt указывает, нужно ли печатать файл с данными узлов решетки при кавитации, или файл с частицами жидкости с указанием их фазы и принадлежности к кластеру при кристаллизации.


### Параметры порядка
Для расчета всех новых параметров порядка необходимо в файле params.txt указать значения ряда переменных.
* CavVolume - параметр порядка, рассчитываемый как размер наибольшей полости. Система разбивается "сеткой", количество ячеек сетки в одном пространственном направлении задается параметром NUMBER_OF_CELLS_1D. Для каждой частицы системы определяется число ее ближайших соседей. Две частицы считаются ближайшими соседями, если расстояние между ними меньше R_NEAREST_NEIGHBOURS. Если число соседей частицы болшье или равно NUM_NEIGHBOURS_LIQUID, то она считается принадлежащей жидкой фазы. Если на росстоянии меньшем R_SEP от ячейки сетка есть хотя бы одна частица жидкой фазы, то и ячейка считается принадлежащей жидкости. Далее находится наибольший кластер связанных ячеек, принадлжеащих газовой фазе. Ячейки связаны, если соприкасаются, то есть имеют хотя бы одну общую вершину.

* CrystVolumeTF
* CrystVolumeLD
### Установка

* Скопировать файлы
```
git clone https://github.com/itpuroran/DFFS.git
```

* Изменить модуль установки переменных окружения на кластере ИММ
```
mpiset 6
```

* Компиляция
```
cd DFFS
mkdir build; cd build
cmake -DLammps=yes ..
make
```
исполняемый файл появится в папке build.

* На этом шаге можно добавить в Lammps термостат Лаве-Андерсена (https://hypermd.wordpress.com/molecular-simulations/linear-motion/thermostat/). Скопировать файлы fix_temp_andersen.cpp и fix_temp_andersen.h в папку "build/hooks/lammps/lammps-download-prefix/src/lammps-download/src". В этой же папке имзенить файл "Makefile.list": вписать fix_temp_andersen.cpp и fix_temp_andersen.h в строки 10 и 12 соответственно. После этого перекомпилировать:

```
make
```

### Запуск
Примеры запуска