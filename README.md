# socketprogrammingerrordetection


Bu projede veri iletişiminde kullanılan hata tespit yöntemleri, C dili ve socket programlama kullanılarak uygulamalı olarak gösterilmiştir. Proje üç bağımsız programdan oluşmaktadır. Client1 gönderici olarak görev yapar ve kullanıcıdan aldığı metin için seçilen hata tespit yöntemine göre kontrol bilgisi üretir. Server ara düğüm olarak çalışır ve Client1’den gelen veriyi bilinçli olarak bozarak gerçek hayattaki iletim hatalarını simüle eder. Client2 ise alıcı taraftır ve gelen verinin bozulup bozulmadığını kontrol eder.

Client1 tarafından gönderilen veri tek bir paket halinde iletilmektedir. Paket formatı DATA|METHOD|CONTROL şeklindedir. DATA kullanıcıdan alınan asıl mesajı, METHOD kullanılan hata tespit yöntemini, CONTROL ise bu mesaj için hesaplanan kontrol bilgisini ifade eder. Server bu paketi aldıktan sonra yalnızca DATA kısmına hata uygular, kontrol bilgisini değiştirmez ve paketi Client2’ye iletir. Bu sayede alıcı tarafta veri bütünlüğünün bozulup bozulmadığı test edilebilir.

Projede Parity, 2D Parity, CRC16, Hamming Code ve Internet Checksum olmak üzere beş farklı hata tespit yöntemi uygulanmıştır. Client2 gelen paketi parçalara ayırır, seçilen yönteme göre kontrol bilgisini yeniden hesaplar ve gönderilen kontrol bilgisi ile karşılaştırır. Eğer iki kontrol bilgisi aynıysa veri doğru kabul edilir, farklıysa veri bozulmuş olarak değerlendirilir.

Programların doğru çalışabilmesi için belirli bir çalıştırma sırası gereklidir. Öncelikle Client2 çalıştırılır çünkü bu program dinleyici konumundadır. Ardından Server başlatılır ve son olarak Client1 çalıştırılarak veri gönderimi yapılır. Yanlış sırada çalıştırma yapılması durumunda bağlantı hataları oluşmaktadır.

Proje Linux tabanlı socket kütüphanelerini kullandığı için Windows ortamında WSL üzerinden Ubuntu kullanılarak geliştirilmiş ve test edilmiştir. Amaç, farklı hata tespit yöntemlerinin çeşitli hata türleri karşısındaki davranışlarını gözlemlemek ve veri iletişiminde hata kontrolünün nasıl gerçekleştirildiğini pratik olarak göstermektir.
