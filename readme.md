### Парсинг .torrent файла

Нужно реализовать функцию `TorrentFile LoadTorrentFile(const std::string& filename)`.
Данная функция парсит .torrent файл и загружает информацию из него в структуру `TorrentFile`.
Как устроен .torrent файл, можно почитать в открытых источниках.
После парсинга файла нужно также заполнить поле `infoHash`, которое не хранится в файле в явном виде и должно быть вычислено.

Данные из файла и `infoHash` будут использованы для запроса пиров у торрент-трекера. Если структура `TorrentFile`
была заполнена правильно, то трекер найдет нужную раздачу в своей базе и ответит списком пиров.
Если данные неверны, то сервер ответит ошибкой.

Для заполнения поля `infoHash` потребуется вычислять `SHA1` хеш-сумму.


### Получение списка пиров у torrent трекера

Нужно реализовать класс `TorrentTracker`, с помощью которого происходит запрос списка пиров.
Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
Для получения списка пиров понадобится парсить ответ трекера в формате bencode, так как .torrent файлы имеют такой же формат.

### Подключение по TCP-сокету

Нужно реализовать класс `TcpConnect`, с помощью которого происходит подключение к пирам по протоколу TCP.
Для общения с пирами используется протокол TCP.
Чтобы работать с этим протоколом, в unix-like системах существует стандартное API. Необходимую информацию о работе с ним
можно узнать в файле tcp_connect.h.

### Подключение к пирам по протоколу torrent

Нужно реализовать класс `PeerConnect`, с помощью которого происходит общение с пиром.
На данный момент требуется реализовать не весь протокол общения с пиром, а только первые сообщения, которые передаются
после установления сетевого соединения -- handshake.
Хендшейк состоит из трех этапов:
- Послать и получить в ответ непосредственно сообщение-хендшейк https://wiki.theory.org/BitTorrentSpecification#Handshake
- Получить `bitfield` с описанием частей файла, которые доступны для скачивания у пира
- Отправить сообщение `interested`, чтобы дать знать пиру, что мы будем запрашивать у него части файла, как только пир нам позволит (отправит нам `unchoke`)
 
Опционально вместо `bitfield` пир может сразу прислать сообщение `unchoke`.
В таком случае надо все равно отправить `interestred`.


Дополнительная информация:
- Что такое торрент https://ru.wikipedia.org/wiki/BitTorrent
- .torrent файл https://ru.wikipedia.org/wiki/.torrent
- Подробное описание протокола http://www.bittorrent.org/beps/bep_0003.html
- Формат компактного ответа трекера http://www.bittorrent.org/beps/bep_0023.html