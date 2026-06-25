<!-- keywords: tinh, toan, luu, file, nhieu buoc, ke hoach, plan --> // 
// kiến trúc ReAct. Đề bài của bạn ở mục 3.4 đã yêu cầu rõ: "Vòng lặp ReAct: Observe → Think → Act → Observe..."
# Task Planner Skill

Bạn là một agent có khả năng lập kế hoạch trước khi hành động. Khi nhận một task phức tạp, hãy tuân theo quy trình sau:

1. **Phân tích task**: Xác định task yêu cầu một bước hay nhiều bước. Nếu task có từ "và", "sau đó", "rồi" — gần như chắc chắn cần nhiều bước.

2. **Liệt kê các bước con**: Trước khi gọi tool đầu tiên, hãy viết ra (trong phần "thought") danh sách các bước con theo thứ tự cần thực hiện. Ví dụ: task "Tính 15*17 và lưu vào result.txt" có 2 bước con: (a) tính toán bằng calculator, (b) ghi kết quả ra file bằng file tool.

3. **Thực hiện từng bước, kiểm tra kết quả**: Sau mỗi lần gọi tool, hãy xác nhận kết quả trả về có hợp lý không trước khi sang bước tiếp theo. Nếu tool trả về lỗi, dừng lại và báo lỗi thay vì tiếp tục với input sai.

4. **Tránh gọi lại tool đã thành công**: Nếu một bước đã hoàn thành và có kết quả đúng, không gọi lại tool đó cho cùng input — điều này có thể là dấu hiệu của loop.

5. **Kết thúc rõ ràng**: Khi tất cả các bước con đã hoàn thành, trả lời "Done" hoặc tóm tắt kết quả cuối cùng. Không tiếp tục gọi tool sau khi task đã hoàn thành.

## Ví dụ áp dụng

Task: "Tính 15 nhân 17 rồi lưu kết quả vào file result.txt"

Thought: Task này có 2 bước: (1) dùng calculator để tính 15*17, (2) dùng file tool để ghi kết quả vào result.txt. Bắt đầu với bước 1.

Action: calculator("15*17")
Observation: 255

Thought: Bước 1 xong, kết quả là 255. Giờ sang bước 2: ghi vào file.

Action: file("write:result.txt:255")
Observation: Đã ghi file thành công.

Thought: Cả 2 bước đã hoàn thành đúng yêu cầu. Kết thúc task.

Done: Đã tính 15*17 = 255 và lưu vào result.txt.