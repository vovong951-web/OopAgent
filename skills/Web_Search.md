<!-- keywords: tim kiem, search, thong tin, tra cuu, internet, moi nhat, gia, ty gia -->
# Web Search Skill

Bạn là một agent biết khi nào cần tra cứu thông tin từ internet thay vì tự suy đoán. Khi nhận task có thể cần thông tin cập nhật hoặc nằm ngoài kiến thức sẵn có, hãy tuân theo quy trình sau:

1. **Xác định có thực sự cần search hay không**: Nếu task hỏi về thông tin cố định, có thể tự suy luận được (ví dụ phép tính, định nghĩa cơ bản), không cần gọi web_search. Chỉ search khi task cần dữ liệu thay đổi theo thời gian (giá cả, tin tức, thời tiết, sự kiện mới) hoặc thông tin cụ thể bạn không chắc chắn.

2. **Viết query ngắn gọn, đúng trọng tâm**: Không gửi nguyên văn câu hỏi dài của người dùng làm query. Trích ra từ khóa chính. Ví dụ task "Giá vàng hôm nay là bao nhiêu, lưu vào file gia_vang.txt" → query nên là "giá vàng hôm nay", không phải nguyên câu.

3. **Đọc kết quả trả về có chọn lọc**: Kết quả search thường trả về nhiều đoạn text. Chỉ lấy phần liên quan trực tiếp đến câu hỏi, không copy nguyên toàn bộ kết quả vào câu trả lời cuối.

4. **Không search lặp lại cùng một query**: Nếu lần search đầu đã có kết quả hợp lý, không gọi lại web_search với query giống hệt hoặc gần giống — đây có thể là dấu hiệu loop. Nếu kết quả không đủ, thử query khác với từ khóa cụ thể hơn, tối đa 2 lần.

5. **Nếu không tìm được thông tin sau 2 lần thử**: Dừng lại, báo rõ cho người dùng là không tìm được thông tin cần thiết, không bịa ra câu trả lời.

## Ví dụ áp dụng

Task: "Tìm tỷ giá USD sang VND hôm nay và lưu vào file tygia.txt"

Thought: Đây là thông tin thay đổi theo thời gian, cần search thay vì tự trả lời. Query nên ngắn gọn: "tỷ giá USD VND hôm nay".

Action: web_search("ty gia USD VND hom nay")
Observation: "1 USD = 25,470 VND (cập nhật hôm nay)"

Thought: Kết quả rõ ràng, đủ thông tin. Sang bước lưu file, không cần search thêm.

Action: file("write:tygia.txt:1 USD = 25470 VND")
Observation: Đã ghi file thành công.

Done: Đã tìm tỷ giá USD/VND hôm nay (25,470 VND) và lưu vào tygia.txt.