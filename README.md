Это версия программы SSAGES (https://github.com/SSAGESproject/SSAGES), адаптированная для расчетов частоты нуклеации при кристаллизации и кавитации жидкости методом Direct Forward Flux Sampling (DFFS). Статья с описанием алгоритма находится в файле allen2009.pdf.

### Отличия от базовой версии

* Добавлены три параметра порядка: CavVolume - объем кавитационной полости, CrystVolumeTF - число частиц в кристаллическом зародыше (на основе https://github.com/jpmit/orderparams)
* Добавлена возможность перезапуска расчета (эта функция заявлена в документации SSAGES, но не релизованна). Во входном файле формата json необходимо указать дополнительный параметр NLastSuccessful - число сохраненных конфигураций на последней достигнутой поверхности.
* Параллельный расчет (на один walker может приходиться более 1го процесора).
* Данные о пересечении промежуточных поверхностей записываются в отдельные файлы в папке outputs.
* В файлы fail-... для экономии места на диске пишется только шапка.



### Параметры порядка
Для расчета всех новых параметров порядка необходимо в файле order_parameter.txt указать значения ряда переменных.
* CavVolume - параметр порядка, рассчитываемый как размер наибольшей полости. Система разбивается "сеткой", количество ячеек сетки в одном пространственном направлении задается параметром **num_cells_1d**. Для каждой частицы системы определяется число ее ближайших соседей. Две частицы считаются ближайшими соседями, если расстояние между ними меньше **r_nearest_neighbours**. Если число соседей частицы больше или равно **num_neighbours_liquid**, то она считается принадлежащей жидкой фазы. Если на расстоянии меньшем **latt_sep** от ячейки сетка есть хотя бы одна частица жидкой фазы, то и ячейка считается принадлежащей жидкости. Далее находится наибольший кластер связанных ячеек, принадлежащих газовой фазе. Ячейки связаны, если соприкасаются, то есть имеют хотя бы одну общую вершину. 
Параметр **write_results** в файле order_parameter.txt указывает, нужно ли печатать значения параметра порядка. Параметр **write_struct** в файле params.txt указывает, нужно ли печатать файл с данными узлов решетки при кавитации, или файл с частицами жидкости с указанием их фазы и принадлежности к кластеру при кристаллизации (использовать только для тестов!).

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
cmake -DLAMMPS=yes ..
make
```
исполняемый файл появится в папке build.

* На этом шаге можно добавить в Lammps термостат Лаве-Андерсена (https://hypermd.wordpress.com/molecular-simulations/linear-motion/thermostat/). Скопировать файлы fix_temp_andersen.cpp и fix_temp_andersen.h в папку "build/hooks/lammps/lammps-download-prefix/src/lammps-download/src". В этой же папке имзенить файл "Makefile.list": вписать fix_temp_andersen.cpp и fix_temp_andersen.h в строки 10 и 12 соответственно. После этого перекомпилировать:

```
make
```

### Запуск

Пример запуска расчета кавитации находится в папке DFFS/Examples/User/ForwardFlux/LAMMPS/cavitation.
* создать ссылку на исполняемый файл ssages (путь для ссылки записан в файле ssages)

* задать требуемые парметры в файлах input.json, FF_Input_Generator.py, order_parameter.txt

* запуск
    ```
    mpiset 6
    python FF_Input_Generator.py
    sbatch -n 8 -t 15 --wrap='orterun ./ssages Input_end.json'

    ```

### Список парметров в файле input.json:

* **walkers** - чило независимых систем метастабильной системы

* **input** - имя шаблона входного файла для lammps
* **CVs** - тип параметра порядка (для кавитации CavVolume)
* **nInterfaces** - количество разделяющих поверхностей
* **interfaces** - расположение поверхностей (формируется в скрипте FF_Input_Generator.py)
* **N0Target** - число конфигураций, сохраняемых на нулевой поверхности
* **NLastSuccessful** - число конфигураций, сохраненных на последней достигнутой поверхности, при первом запуске должно быть равно N0Target
* **trials** - количество пробных запусков с каждой поверхности
* **computeInitialFlux** - нужно ли считать начальный поток
* **saveTrajectories** - сохранять ли траектории (требует много памяти и не работает в многопроцессорном режиме)
* **currentInterface** - номер последней достигнутой поверхности
* **outputDirectoryName** - папка для сохранения снимков системы

В скрипте FF_Input_Generator.py необходимо задать параметры в строках 110-115.
```
# User must set these variables
nWalkers = 32
input_filename = "cr_nvt"
nsurf = 51
interfaces = np.linspace(1, 101, nsurf ) 
trials = np.empty(nsurf , dtype=int)
trials.fill(100)
# Use if you have many equally-spaced interfaces
```
В приведенном примере после запуска скрипт создаст 32 стартовых файла для Lammps, будут заданы значения для 51 поверхности со значениями от 1 до 101. Число пробных запусков с каждой поверхности будет равно 100.
