#include "vim_registers.h"

#include <QClipboard>
#include <QGuiApplication>

namespace Ui {
namespace Vim {

namespace {

/**
 * @brief Регистр, в который попадает всё скопированное и удалённое
 */
const QChar kUnnamedRegisterName = '"';

/**
 * @brief Регистр, в который попадает последний скопированный фрагмент
 */
const QChar kYankRegisterName = '0';

/**
 * @brief Регистр, в который попадает последнее удаление в пределах строки
 */
const QChar kSmallDeleteRegisterName = '-';

/**
 * @brief Регистр, из которого удалённый текст никогда не вернуть
 */
const QChar kBlackHoleRegisterName = '_';

/**
 * @brief Регистры системного буфера обмена
 */
bool isClipboardRegister(QChar _name)
{
    return _name == '+' || _name == '*';
}

/**
 * @brief Добавить перевод строки в конец построчного фрагмента
 */
QString normalizedText(const QString& _text, bool _linewise)
{
    if (!_linewise || _text.endsWith('\n')) {
        return _text;
    }

    return _text + '\n';
}

} // namespace


bool RegisterContent::isEmpty() const
{
    return text.isEmpty();
}

bool Registers::isValidName(QChar _name)
{
    const auto code = _name.unicode();
    return (code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z')
        || (code >= '0' && code <= '9') || isClipboardRegister(_name)
        || _name == kUnnamedRegisterName || _name == kBlackHoleRegisterName
        || _name == kSmallDeleteRegisterName;
}

void Registers::setYankedContent(QChar _name, const RegisterContent& _content)
{
    if (_name == kBlackHoleRegisterName) {
        return;
    }

    if (_name.isNull() || _name == kUnnamedRegisterName) {
        const auto content = setContent(kYankRegisterName, _content);
        setContent(kUnnamedRegisterName, content);
        syncToClipboard(content);
        return;
    }

    const auto content = setContent(_name, _content);
    m_registers[kUnnamedRegisterName] = content;
}

void Registers::setDeletedContent(QChar _name, const RegisterContent& _content, bool _toNumbered)
{
    if (_name == kBlackHoleRegisterName) {
        return;
    }

    RegisterContent content = _content;
    if (!_name.isNull() && _name != kUnnamedRegisterName) {
        content = setContent(_name, _content);
    }

    //
    // ... многострочные удаления сдвигают историю нумерованных регистров, а удаления в пределах
    //     строки складываются в отдельный регистр
    //
    const bool isMultiline = _content.linewise || _content.text.contains('\n');
    if (isMultiline || _toNumbered) {
        for (int index = 9; index > 1; --index) {
            const auto previous = m_registers.value(QChar('0' + index - 1));
            if (!previous.isEmpty()) {
                m_registers[QChar('0' + index)] = previous;
            }
        }
        setContent('1', _content);
    }
    if (!isMultiline && (_name.isNull() || _name == kUnnamedRegisterName)) {
        setContent(kSmallDeleteRegisterName, _content);
    }

    m_registers[kUnnamedRegisterName] = content;
    if (_name.isNull() || _name == kUnnamedRegisterName || isClipboardRegister(_name)) {
        syncToClipboard(content);
    }
}

RegisterContent Registers::content(QChar _name) const
{
    if (_name == kBlackHoleRegisterName) {
        return {};
    }

    if (isClipboardRegister(_name)) {
        return clipboardContent();
    }

    if (_name.isNull() || _name == kUnnamedRegisterName) {
        //
        // ... если буфер обмена сменился в другом приложении, то вставляем именно его содержимое
        // @note Пустой буфер обмена игнорируем, иначе регистр перестанет работать в окружениях,
        //       в которых буфер обмена недоступен
        //
        const auto clipboard = clipboardContent();
        if (!clipboard.isEmpty() && clipboard.text != m_clipboardContent.text) {
            return clipboard;
        }
        return m_registers.value(kUnnamedRegisterName);
    }

    return m_registers.value(_name.toLower());
}

RegisterContent Registers::setContent(QChar _name, const RegisterContent& _content)
{
    auto content = _content;
    content.text = normalizedText(content.text, content.linewise);

    if (isClipboardRegister(_name)) {
        syncToClipboard(content);
        return content;
    }

    //
    // ... заглавная буква в имени регистра означает, что текст нужно дописать к уже имеющемуся,
    //     при этом построчный фрагмент делает построчным и весь регистр
    //
    if (_name.isUpper()) {
        const auto existingContent = m_registers.value(_name.toLower());
        if (!existingContent.isEmpty()) {
            const bool linewise = existingContent.linewise || content.linewise;
            auto text = existingContent.text;
            if (linewise && !text.endsWith('\n')) {
                text += '\n';
            }
            content.text = normalizedText(text + content.text, linewise);
            content.linewise = linewise;
        }
    }

    m_registers[_name.toLower()] = content;
    return content;
}

void Registers::syncToClipboard(const RegisterContent& _content)
{
    m_clipboardContent = _content;
    QGuiApplication::clipboard()->setText(_content.text);
}

RegisterContent Registers::clipboardContent() const
{
    auto text = QGuiApplication::clipboard()->text();
    if (text == m_clipboardContent.text) {
        return m_clipboardContent;
    }

    //
    // ... в других приложениях, да и в самих редакторах, строки могут разделяться по-разному
    //
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace('\r', '\n');

    //
    // ... текст, скопированный в другом приложении целыми строками, вставляем построчно
    //
    return { text, text.endsWith('\n') };
}

} // namespace Vim
} // namespace Ui
