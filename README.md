1. Печатает введённую строку и выходит
2. Печатает введённую строку в цикле и выходит по Ctrl+D
3. Добавим команду для выхода (exit и \q)
===
4. Добавим историю введённых команд и её сохранения в файл
5. Добавим команду echo
6. Добавим проверку введённой команды
7. Добавим команду по выводу переменной окружения (\e $PATH)
8. Выполняем указанный бинарник
9. По сигналу SIGHUP вывести "Configuration reloaded"
10. По `\l /dev/sda` получить информацию о разделах в системе
11. Поает введёподключить VFS в /tmp/vfs со списком задач в планировщике
12. Пооку в цикле и выхполучить дамп памяти процесса
Вот пример задания на shell


Список претензий :
1) В 8 задании запуск идет через системный вызов на C, что не правильно?
2) В 10 также системные вызовы, их не должно быть
3) В 12 dump пустой, такого быть не должно



Уточнения для 10-го задания 
Тогда можно переформулировать на "Определить является ли диск загрузочным". У них должна быть сигнатура 55AA. Т.е. пользователь вводит sda, sdb и т.п., а в шелл нужно смотреть первый сектор введённого диска.
3) Не выходит по \q
7) Было бы не плохо добавить $
10) Не получается получить информацию о системе
11) Доделать
12) Не работает
